# 1.6 GDPS's mod
This modification, supporting Android 2.2+ and jailbroken iOS 4.3-10.3.3, provides the following changes to Geometry Dash 1.6 that are in use on 1.6 GDPS:
- Account creation and authorization
- Saving and loading progress
- Copying levels
- Rate menu for moderators
- Level completion's progress shown as a percentage label
- Support for custom songs from muzmo, which can be used in levels, and also be set as the menu or practice song
- Setting a song offset for your level
- RGB color picker in the editor
- Removed the border at 1000 blocks in the editor
- Exporting and importing gmd files
- Level length is shown in level settings

## Screenshots
<img width="2400" height="1080" alt="1" src="https://github.com/user-attachments/assets/908d5667-4d55-44ca-8f94-1e4415da282e" />
<img width="960" height="640" alt="1-2" src="https://github.com/user-attachments/assets/059215c1-cfcc-4d63-8b78-3612757dc32b" />
<img width="2400" height="1080" alt="2" src="https://github.com/user-attachments/assets/557deb44-5402-4bf2-a619-a85fe6c2aa41" />
<img width="960" height="640" alt="2-2" src="https://github.com/user-attachments/assets/82165b18-e058-4614-bf43-5eda222f5e25" />
<img width="2400" height="1080" alt="3" src="https://github.com/user-attachments/assets/d900bb4b-6467-4e25-ae41-264a41a25dfe" />
<img width="960" height="640" alt="3-2" src="https://github.com/user-attachments/assets/376cc36d-c7f4-4546-911b-1e33c3e1b4d0" />
<img width="2400" height="1080" alt="4" src="https://github.com/user-attachments/assets/d9619f86-ce2b-473f-b43a-f2cbe6ce2ba8" />
<img width="960" height="640" alt="4-2" src="https://github.com/user-attachments/assets/e0d03951-a5a6-4d3d-a407-d3c3a0561a06" />

## Libraries used
### Android
- HookZz Feb 2018 (function hooking on armv7)
- subhook (function hooking on x86)
- Jsoup 1.13.1 (webpage parsing)
- Mp3agic 0.8.1 (parsing tags of mp3 files)
- Spongy Castle 1.58.0 (tls 1.2 support for old android)
- OkHttp 1.5.4 (fetching webpages and files)
### iOS
- id3tag (parsing tags of mp3 files)
- wolfssl (tls 1.3 support for old iOS)
- curl (fetching webpages and files)
