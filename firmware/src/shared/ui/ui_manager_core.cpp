#include "ui_manager.h"

#ifdef WAVESHARE_7B
#include <lvgl.h>
#include "display.h"
#include "panel_config.h"
#endif

void UiManager::begin() {
  Serial.println("[ui] Family Hub panel UI ready (serial fallback / LVGL hook)");
}

void UiManager::showWriteResult(bool ok, const char* action) {
  lastWriteOk_ = ok;
  lastWriteAction_ = action ? action : "";
  lastWriteMs_ = millis();
  renderRequested_ = true;
}

bool UiManager::consumeRenderRequest() {
  bool requested = renderRequested_;
  renderRequested_ = false;
  return requested;
}

void UiManager::requestSync() {
  syncRequested_ = true;
}

bool UiManager::consumeSyncRequest() {
  bool requested = syncRequested_;
  syncRequested_ = false;
  return requested;
}

const char* UiManager::badgeLabel(const PanelStatus& status, bool stale) const {
  if (status.conn == ConnState::WifiDisconnected) return "OFFLINE";
  if (status.conn == ConnState::ServerOffline) return "OFFLINE";
  if (stale || status.conn == ConnState::Stale) return "STALE DATA";
  return "ONLINE";
}

void UiManager::renderStatusBar(const PanelStatus& status, bool stale) {
  const char* badge = badgeLabel(status, stale);

  Serial.println("--- STATUS ---");
  Serial.printf("[badge] %s\n", badge);
  Serial.printf("conn=%s stale=%s rssi=%d host=%s:%u fw=%s device=%s\n",
                badge, stale ? "yes" : "no", status.wifiRssi,
                status.serverHost.c_str(), status.serverPort,
                status.firmwareVersion.c_str(), status.deviceId.c_str());
  if (status.lastError.length()) Serial.printf("lastError=%s\n", status.lastError.c_str());
  if (status.lastSyncMs) Serial.printf("lastSyncMs=%lu\n", status.lastSyncMs);

  if (lastWriteMs_ && millis() - lastWriteMs_ < 8000) {
    Serial.printf("lastWrite=%s %s\n", lastWriteOk_ ? "OK" : "FAILED", lastWriteAction_.c_str());
  }

#ifdef WAVESHARE_7B
  // Status badge + content are drawn via LVGL in lvglUpdate(); see render().
#elif defined(ELECROW_7)
  // LVGL (LovyanGFX): same badge/footer contract as Waveshare — init in panel bring-up, not here yet.
#endif
}

#ifdef WAVESHARE_7B
void UiManager::resetLvglPointers() {
  headerBar_=brandTitle_=brandSub_=syncButton_=syncLabel_=statusPill_=statusDot_=statusText_=nullptr;
  pageTitle_=pageMeta_=toast_=choreList_=modal_=modalTitle_=modalBody_=modalStatus_=nullptr;
  modalPrimaryBtn_=modalPrimaryLabel_=modalSecondaryBtn_=modalSecondaryLabel_=nullptr;
  navBg_=settingsPanel_=settingsStatus_=settingsHost_=settingsPort_=settingsToken_=settingsKeyboard_=nullptr;
  appPanel_=appUrlLabel_=appHintLabel_=appQr_=nullptr;
  groceryBoard_=nullptr;
  settingsSeeded_=false;
  appQrUrl_="";
  for(int i=0;i<4;++i) cards_[i]=cardTitles_[i]=cardBodies_[i]=cardBodyLabels_[i]=nullptr;
  for(int i=0;i<kNavTabCount;++i) navButtons_[i]=navLabels_[i]=nullptr;
  for(int i=0;i<kMaxChoreRows;++i) choreRowBtns_[i]=choreRowLabels_[i]=nullptr;
  for(int c=0;c<kGroceryCols;++c) {
    groceryColTitles_[c]=nullptr;
    for(int r=0;r<kGroceryRowsPerCol;++r) {
      groceryRowBtns_[c][r]=groceryRowLabels_[c][r]=nullptr;
      groceryRowIds_[c][r][0]='\0';
    }
  }
  modalMode_=0;
}
#endif // WAVESHARE_7B
