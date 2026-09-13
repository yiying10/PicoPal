# PicoPal

PicoPal 是一個以 ESP32-S3、128×64 SSD1306 OLED 為核心的桌面夥伴。

它包含三個以搖桿切換的畫面：

- Timer：在 Timer 頁按 GPIO15 按鈕開始／暫停，按下搖桿可停止並重置；畫面以三位分鐘：秒顯示累積專注時間。
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

**## Host bridge**

`companion/picopal_bridge.py` 透過 USB serial 傳送 versioned `PICO/1` 狀態事件：

```bash
python3 -m venv companion/.venv
companion/.venv/bin/pip install -r companion/requirements.txt
companion/.venv/bin/python companion/picopal_bridge.py --port /dev/cu.usbmodemXXXX coding
companion/.venv/bin/python companion/picopal_bridge.py --port /dev/cu.usbmodemXXXX done
```

另支援 `error`、`recovered` 與 `disconnected`。實際 serial port 可用 `ls /dev/cu.usbmodem*` 查詢。


## 手機 API

- `GET /api/images`：取得圖片列表與目前選取 ID。
- `GET /api/images/current`：取得目前圖片的 1024-byte 1-bit framebuffer。
- `POST /api/images/current`：新增圖片；body 為 1024 bytes，並帶 `X-Image-CRC32` header。
- `POST /api/images/select?id=ID`：選取圖片。
- `DELETE /api/images/ID`：刪除圖片。
