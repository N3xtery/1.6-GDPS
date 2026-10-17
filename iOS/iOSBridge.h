// main.cpp
#ifdef __cplusplus
extern "C" {
#endif
extern void cloneLevel();
extern void iOSSetupNative();
extern void setColorNative(int r, int g, int b);
extern void buildLoadingDialog(const char* id);
extern void showMessageBox(const char* title, const char* msg);
extern void importGmd(const char* pathC);
extern void exportGmd(const char* pathC, bool gmd);
extern const char* getCurrentLevelName();
extern int getCurrentLevelLength();

// network.cpp
extern void sendAuthRequest(const char* username, const char* password, bool reg);
extern void sendRateRequest(const char* username, const char* password, int stars, bool featured);
extern void sendSaveRequest(const char* usernameC, const char* passwordC);
extern void sendLoadRequest(const char* username, const char* password, bool noLevelsLocal);
extern void sendReqRequest(const char* username, const char* password);

// songs.cpp
extern void setCustomSong(const char* id);
extern void setOffset(float offset);
extern void updateMenuPractSongNative(bool isMenuSong, const char* songPath, bool updateMenuNow);
extern void updateSongsPathNative(const char* songPath);
extern void playLevel();

// percentage.cpp
extern void updatePercentageNative(int percentage);

// OverlayUI.m
extern void iOSBuildOverlay();
extern void showGroup(const char* button, bool visible);
extern void setOriginalText(int orig);
extern void setOrigSongBtnVisible();
extern void disableRateBtn();

// SongsDialog.m
extern void setCurrentSongID(const char* value);
extern void setCurrentSongOffset(float value);

// RateMenu.m
extern void setStarsFeatured(int starsLocal, bool featuredLocal);

// ColorPicker.m
extern void setColor(int r, int g, int b);
#ifdef __cplusplus
}
#endif
