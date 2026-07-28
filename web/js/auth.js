'use strict';

/**
 * Clerk browser bootstrap for Family Hub (vanilla JS, no bundler).
 * Clerk JS v6 + @clerk/ui (custom domain / Frontend API from publishable key).
 */
(function () {
  let clerk = null;
  let authMe = null;
  let config = { clerkEnabled: false, publishableKey: '' };
  let readyPromise = null;

  async function loadConfig() {
    const res = await fetch('/api/auth/config');
    if (!res.ok) throw new Error('auth-config-failed');
    config = await res.json();
    return config;
  }

  function frontendApiFromPublishableKey(publishableKey) {
    try {
      const payload = publishableKey.replace(/^pk_(test|live)_/, '');
      const decoded = atob(payload);
      const frontendApi = decoded.split('$')[0];
      if (frontendApi && frontendApi.includes('.')) return frontendApi;
    } catch (_) {
      /* ignore */
    }
    return '';
  }

  function waitForClerkGlobal(timeoutMs = 20000) {
    return new Promise((resolve, reject) => {
      const start = Date.now();
      (function tick() {
        if (window.Clerk) return resolve(window.Clerk);
        if (Date.now() - start > timeoutMs) {
          return reject(new Error('clerk-global-timeout'));
        }
        setTimeout(tick, 40);
      })();
    });
  }

  function loadScript(src, attrs = {}) {
    return new Promise((resolve, reject) => {
      const existing = document.querySelector(`script[src="${src}"]`);
      if (existing) {
        if (existing.dataset.loaded === '1') return resolve();
        existing.addEventListener('load', () => resolve(), { once: true });
        existing.addEventListener('error', () => reject(new Error(`script-load-failed:${src}`)), {
          once: true,
        });
        return;
      }
      const s = document.createElement('script');
      s.async = true;
      s.defer = true;
      s.crossOrigin = 'anonymous';
      s.src = src;
      Object.entries(attrs).forEach(([k, v]) => {
        if (k.startsWith('data')) s.setAttribute(k, v);
        else s[k] = v;
      });
      s.onload = () => {
        s.dataset.loaded = '1';
        resolve();
      };
      s.onerror = () => reject(new Error(`script-load-failed:${src}`));
      document.head.appendChild(s);
    });
  }

  function clerkUiAvailable(instance) {
    if (!instance) return false;
    try {
      if (typeof instance.assertComponentsReady === 'function') {
        instance.assertComponentsReady();
        return true;
      }
    } catch (_) {
      return false;
    }
    return true;
  }

  function buildLoadOpts(publishableKey) {
    const home = `${window.location.origin}/`;
    // Do NOT set signInForceRedirectUrl — forced "/" redirects loop with the login gate.
    const loadOpts = {
      publishableKey,
      afterSignOutUrl: home,
    };
    if (window.__internal_ClerkUICtor) {
      loadOpts.ui = { ClerkUI: window.__internal_ClerkUICtor };
    }
    return loadOpts;
  }

  async function loadClerkScript(publishableKey) {
    if (window.Clerk && window.Clerk.loaded) return window.Clerk;

    const frontendApi = frontendApiFromPublishableKey(publishableKey);
    const base = frontendApi
      ? `https://${frontendApi}`
      : 'https://cdn.jsdelivr.net/npm';
    const uiSrc = frontendApi
      ? `${base}/npm/@clerk/ui@1/dist/ui.browser.js`
      : 'https://cdn.jsdelivr.net/npm/@clerk/ui@1/dist/ui.browser.js';
    const clerkSrc = frontendApi
      ? `${base}/npm/@clerk/clerk-js@6/dist/clerk.browser.js`
      : 'https://cdn.jsdelivr.net/npm/@clerk/clerk-js@6/dist/clerk.browser.js';

    // UI first, then clerk-js. The publishable-key attribute is required for the
    // CDN build to create window.Clerk (without it, Clerk never appears).
    await loadScript(uiSrc);
    await loadScript(clerkSrc, {
      'data-clerk-publishable-key': publishableKey,
    });

    const instance = await waitForClerkGlobal();
    if (typeof instance.load === 'function' && !instance.loaded) {
      await instance.load(buildLoadOpts(publishableKey));
    } else if (!clerkUiAvailable(instance) && typeof instance.load === 'function') {
      // Auto-load may have skipped UI — retry with ClerkUI attached.
      try {
        await instance.load(buildLoadOpts(publishableKey));
      } catch (_) {
        /* hosted fallback used at mount time */
      }
    }
    return instance;
  }

  async function init() {
    if (readyPromise) return readyPromise;
    readyPromise = (async () => {
      try {
        await loadConfig();
      } catch (_) {
        config = { clerkEnabled: false, publishableKey: '' };
        return { clerkEnabled: false };
      }
      if (!config.clerkEnabled || !config.publishableKey) {
        return { clerkEnabled: false };
      }
      clerk = await loadClerkScript(config.publishableKey);
      return { clerkEnabled: true, clerk };
    })();
    return readyPromise;
  }

  async function getToken() {
    await init();
    if (!config.clerkEnabled) return '';
    if (!clerk || !clerk.session) return '';
    try {
      return (await clerk.session.getToken()) || '';
    } catch (_) {
      return '';
    }
  }

  function isSignedIn() {
    if (!clerk) return false;
    if (typeof clerk.isSignedIn === 'function') {
      try {
        return Boolean(clerk.isSignedIn());
      } catch (_) {
        /* fall through */
      }
    }
    if (typeof clerk.isSignedIn === 'boolean') return clerk.isSignedIn;
    return Boolean(clerk.user || clerk.session);
  }

  function looksLikeAuthReturn() {
    try {
      const u = new URL(window.location.href);
      const q = `${u.search}${u.hash}`.toLowerCase();
      return (
        q.includes('__clerk') ||
        q.includes('clerk_status') ||
        q.includes('sso-callback') ||
        q.includes('handshake') ||
        q.includes('created_session')
      );
    } catch (_) {
      return false;
    }
  }

  /**
   * After OAuth / handshake return, Clerk may still be finishing session setup.
   * Wait briefly so the login gate does not mount SignIn and bounce forever.
   * Cold visits skip the long wait so mobile login shows immediately.
   */
  async function settleAuth(timeoutMs) {
    await init();
    if (!config.clerkEnabled || !clerk) return false;
    if (isSignedIn()) return true;

    const waitMs =
      typeof timeoutMs === 'number'
        ? timeoutMs
        : looksLikeAuthReturn()
          ? 2500
          : 350;

    return new Promise((resolve) => {
      let done = false;
      let unsub = () => {};
      const finish = (value) => {
        if (done) return;
        done = true;
        clearTimeout(timer);
        try {
          unsub();
        } catch (_) {
          /* ignore */
        }
        resolve(value);
      };
      unsub = addListener(() => {
        if (isSignedIn()) finish(true);
      });
      const timer = setTimeout(() => finish(isSignedIn()), waitMs);
      if (isSignedIn()) finish(true);
    });
  }

  async function refreshMe() {
    if (!config.clerkEnabled || !isSignedIn()) {
      authMe = null;
      return null;
    }
    authMe = await API.request('/api/auth/me');
    return authMe;
  }

  async function openSignIn() {
    await init();
    if (!clerk) return;
    if (clerkUiAvailable(clerk) && typeof clerk.openSignIn === 'function') {
      clerk.openSignIn({ routing: 'hash' });
      return;
    }
    if (typeof clerk.redirectToSignIn === 'function') clerk.redirectToSignIn();
  }

  async function openSignUp() {
    await init();
    if (!clerk) return;
    if (clerkUiAvailable(clerk) && typeof clerk.openSignUp === 'function') {
      clerk.openSignUp({ routing: 'hash' });
      return;
    }
    if (typeof clerk.redirectToSignUp === 'function') clerk.redirectToSignUp();
    else openSignIn();
  }

  function mountHostedAuthFallback(el, mode) {
    if (!el) return;
    const label = mode === 'sign-up' ? 'Create account' : 'Continue to sign in';
    el.innerHTML = '';
    const btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'btn';
    btn.id = 'fhHostedAuthBtn';
    btn.textContent = label;
    btn.style.width = '100%';
    btn.addEventListener('click', () => {
      if (mode === 'sign-up') openSignUp();
      else openSignIn();
    });
    const hint = document.createElement('p');
    hint.className = 'muted';
    hint.style.margin = '10px 0 0';
    hint.style.textAlign = 'center';
    hint.style.fontSize = '0.8rem';
    hint.textContent = 'Opens secure Clerk sign-in';
    el.appendChild(btn);
    el.appendChild(hint);
  }

  async function mountUserButton(el) {
    await init();
    if (!clerk || !el || !isSignedIn()) return;
    el.innerHTML = '';
    try {
      if (typeof clerk.mountUserButton === 'function') {
        clerk.mountUserButton(el);
      }
    } catch (_) {
      /* optional */
    }
  }

  function embeddedAuthProps() {
    // Hash routing works for vanilla embeds; avoid forceRedirectUrl reload loops.
    return {
      routing: 'hash',
    };
  }

  async function mountSignIn(el) {
    await init();
    if (!clerk || !el) return;
    if (isSignedIn()) return;
    try {
      if (typeof clerk.unmountSignIn === 'function') clerk.unmountSignIn(el);
    } catch (_) { /* ignore */ }
    el.innerHTML = '';
    if (!clerkUiAvailable(clerk)) {
      mountHostedAuthFallback(el, 'sign-in');
      return;
    }
    if (typeof clerk.mountSignIn === 'function') {
      clerk.mountSignIn(el, embeddedAuthProps());
    }
  }

  async function mountSignUp(el) {
    await init();
    if (!clerk || !el) return;
    if (isSignedIn()) return;
    try {
      if (typeof clerk.unmountSignUp === 'function') clerk.unmountSignUp(el);
    } catch (_) { /* ignore */ }
    el.innerHTML = '';
    if (!clerkUiAvailable(clerk)) {
      mountHostedAuthFallback(el, 'sign-up');
      return;
    }
    if (typeof clerk.mountSignUp === 'function') {
      clerk.mountSignUp(el, embeddedAuthProps());
    }
  }

  async function unmount(el) {
    if (!clerk || !el) return;
    try {
      if (typeof clerk.unmountSignIn === 'function') clerk.unmountSignIn(el);
    } catch (_) { /* ignore */ }
    try {
      if (typeof clerk.unmountSignUp === 'function') clerk.unmountSignUp(el);
    } catch (_) { /* ignore */ }
    el.innerHTML = '';
  }

  function addListener(cb) {
    if (!clerk || typeof clerk.addListener !== 'function') return () => {};
    // Clerk fires on session/user changes after embedded or redirect sign-in.
    return clerk.addListener((resources) => {
      try {
        cb(resources);
      } catch (_) {
        /* ignore listener errors */
      }
    });
  }

  async function signOut() {
    await init();
    if (clerk && typeof clerk.signOut === 'function') await clerk.signOut();
    authMe = null;
  }

  window.FamilyHubAuth = {
    init,
    settleAuth,
    getConfig: () => config,
    getToken,
    isSignedIn,
    getMe: () => authMe,
    refreshMe,
    openSignIn,
    openSignUp,
    mountSignIn,
    mountSignUp,
    unmount,
    mountUserButton,
    addListener,
    signOut,
    getClerk: () => clerk,
  };
})();
