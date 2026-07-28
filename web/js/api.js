'use strict';

const WRITE_TOKEN_KEY = 'familyHubWriteToken';
const FLASH_DEFAULT_MS = 2800;

function esc(s) {
  return String(s ?? '')
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function flashEl() {
  return document.getElementById('toast') || document.getElementById('flash');
}

function showFlash(message, kind = 'error', el = flashEl()) {
  if (!el) return;
  el.textContent = message;
  el.className = `toast flash ${kind}`.trim();
  el.classList.remove('hidden');
  clearTimeout(showFlash._t);
  if (kind === 'ok' || kind === 'error') {
    showFlash._t = setTimeout(() => hideFlash(el), FLASH_DEFAULT_MS);
  }
}

function hideFlash(el = flashEl()) {
  if (!el) return;
  clearTimeout(showFlash._t);
  el.className = 'toast flash hidden';
  el.textContent = '';
}

function formatApiError(err) {
  if (!err) return 'Action failed';
  if (err.status === 401 || err.code === 'unauthorized') {
    return err.message || 'Sign in required — use Setup or the Sign in control.';
  }
  if (err.status === 403 || err.code === 'forbidden') {
    return err.message || 'Not allowed for your role.';
  }
  if (err.status === 409 || err.code === 'version-conflict') {
    return err.message || 'Data changed on the server — refresh and try again.';
  }
  return err.message || 'Action failed';
}

const API = {
  base: '',
  lastSync: null,
  online: false,
  esc,
  showFlash,
  hideFlash,
  formatApiError,

  get writeToken() {
    try {
      return localStorage.getItem(WRITE_TOKEN_KEY) || '';
    } catch (_) {
      return '';
    }
  },

  setWriteToken(token) {
    try {
      if (token) localStorage.setItem(WRITE_TOKEN_KEY, token);
      else localStorage.removeItem(WRITE_TOKEN_KEY);
    } catch (_) {
      // localStorage unavailable
    }
  },

  writeHeaders() {
    const token = this.writeToken;
    return token ? { 'x-family-hub-token': token } : {};
  },

  async authHeaders() {
    const headers = { ...this.writeHeaders() };
    if (window.FamilyHubAuth) {
      try {
        const bearer = await FamilyHubAuth.getToken();
        if (bearer) headers.Authorization = `Bearer ${bearer}`;
      } catch (_) {
        /* ignore */
      }
    }
    return headers;
  },

  async request(path, options = {}) {
    const method = (options.method || 'GET').toUpperCase();
    const isWrite = method !== 'GET' && method !== 'HEAD';
    let res;
    try {
      const auth = await this.authHeaders();
      res = await fetch(`${this.base}${path}`, {
        headers: {
          'Content-Type': 'application/json',
          ...(isWrite || path.startsWith('/api/auth/') ? auth : {}),
          // Always send Bearer when available so protected GETs work under Clerk.
          ...(!isWrite && auth.Authorization ? { Authorization: auth.Authorization } : {}),
          ...(options.headers || {}),
        },
        ...options,
      });
    } catch (networkErr) {
      const err = new Error('Cannot reach Family Hub server');
      err.status = 0;
      err.code = 'network';
      err.cause = networkErr;
      throw err;
    }

    const text = await res.text();
    let body = null;
    if (text) {
      try {
        body = JSON.parse(text);
      } catch {
        body = text;
      }
    }

    if (res.status === 401) {
      const err = new Error(
        (body && body.message) ||
          'Sign in required — open Sign in or save a write token in Setup (LAN fallback).'
      );
      err.status = 401;
      err.code = 'unauthorized';
      err.data = body;
      throw err;
    }

    if (res.status === 403) {
      const err = new Error((body && body.message) || 'Not allowed for your role.');
      err.status = 403;
      err.code = 'forbidden';
      err.data = body;
      throw err;
    }

    if (!res.ok) {
      const code = (body && body.error) || `http-${res.status}`;
      const msg =
        (body && (body.message || body.error)) || `HTTP ${res.status}`;
      const err = new Error(msg);
      err.status = res.status;
      err.code = code;
      err.data = body;
      if (res.status === 409 && body && body.error === 'version-conflict') {
        err.code = 'version-conflict';
        err.message = body.message || 'Data changed on the server — refresh and try again.';
      }
      throw err;
    }

    if (res.status === 204) return null;
    return body;
  },

  async health() {
    return this.request('/api/health');
  },

  async dashboardState() {
    const data = await this.request('/api/dashboard-state');
    this.lastSync = new Date();
    this.online = true;
    return data;
  },

  auth: {
    config: () => API.request('/api/auth/config'),
    me: () => API.request('/api/auth/me'),
    linkMember: (memberId) =>
      API.request('/api/auth/link-member', {
        method: 'POST',
        body: JSON.stringify({ memberId }),
      }),
    unlinkMember: (memberId) =>
      API.request('/api/auth/unlink-member', {
        method: 'POST',
        body: JSON.stringify({ memberId }),
      }),
  },

  grocery: {
    list: () => API.request('/api/grocery'),
    create: (body) => API.request('/api/grocery', { method: 'POST', body: JSON.stringify(body) }),
    update: (id, body) => API.request(`/api/grocery/${id}`, { method: 'PATCH', body: JSON.stringify(body) }),
    toggle: (id) => API.request(`/api/grocery/${id}/toggle`, { method: 'POST', body: '{}' }),
    remove: (id) => API.request(`/api/grocery/${id}`, { method: 'DELETE' }),
    setOtherTitle: (otherTitle) =>
      API.request('/api/grocery/meta/other-title', {
        method: 'PUT',
        body: JSON.stringify({ otherTitle }),
      }),
  },

  chores: {
    list: () => API.request('/api/chores'),
    create: (body) => API.request('/api/chores', { method: 'POST', body: JSON.stringify(body) }),
    update: (id, body) => API.request(`/api/chores/${id}`, { method: 'PATCH', body: JSON.stringify(body) }),
    remove: (id) => API.request(`/api/chores/${id}`, { method: 'DELETE' }),
  },

  dinner: {
    get: (date) => API.request(`/api/dinner/${date}`),
    set: (date, body) => API.request(`/api/dinner/${date}`, { method: 'PUT', body: JSON.stringify(body) }),
    week: (start, end) => {
      const params = new URLSearchParams();
      if (start) params.set('start', start);
      if (end) params.set('end', end);
      const qs = params.toString();
      return API.request(`/api/dinner${qs ? `?${qs}` : ''}`);
    },
  },

  notes: {
    list: () => API.request('/api/notes'),
    create: (body) => API.request('/api/notes', { method: 'POST', body: JSON.stringify(body) }),
    update: (id, body) => API.request(`/api/notes/${id}`, { method: 'PATCH', body: JSON.stringify(body) }),
    remove: (id) => API.request(`/api/notes/${id}`, { method: 'DELETE' }),
  },

  members: {
    list: () => API.request('/api/members'),
    create: (body) => API.request('/api/members', { method: 'POST', body: JSON.stringify(body) }),
    update: (id, body) => API.request(`/api/members/${id}`, { method: 'PATCH', body: JSON.stringify(body) }),
    remove: (id) => API.request(`/api/members/${id}`, { method: 'DELETE' }),
  },

  reminders: {
    list: (params = {}) => {
      const qs = new URLSearchParams();
      if (params.entityType) qs.set('entity_type', params.entityType);
      if (params.entityId) qs.set('entity_id', params.entityId);
      if (params.status) qs.set('status', params.status);
      const q = qs.toString();
      return API.request(`/api/reminders${q ? `?${q}` : ''}`);
    },
    create: (body) => API.request('/api/reminders', { method: 'POST', body: JSON.stringify(body) }),
    cancel: (id) => API.request(`/api/reminders/${id}`, { method: 'DELETE' }),
  },

  event(body) {
    return API.request('/api/events', {
      method: 'POST',
      body: JSON.stringify(body),
      headers: { 'x-family-hub-source': 'browser' },
    });
  },
};

window.API = API;
