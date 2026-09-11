# PicoPal

PicoPal 是一個以 ESP32-S3、128×64 SSD1306 OLED 為核心的桌面夥伴。

它包含三個以搖桿切換的畫面：

- Timer：在 Timer 頁按下搖桿，可在開始／暫停間切換；畫面以三位分鐘：秒顯示每天累積的專注時間。
- Pico：參數化眼睛、感測器互動，以及 Codex 任務完成動畫通知。
- Draw：從 iPhone 網頁畫圖、保存最多 100 張，並用搖桿上下瀏覽。

## 專案結構

```text
PicoPal/
├── CMakeLists.txt     # ESP-IDF project 入口
├── main/              # firmware source 與 component 設定
├── web/               # iPhone Focus／Draw 管理介面的原始碼
├── companion/         # Codex 事件 bridge 與開發測試工具
├── tests/             # host-side unit tests 與測試資料
└── README.md
```
