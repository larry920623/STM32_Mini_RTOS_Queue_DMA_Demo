# 先把應用跑起來

## 1. 匯入與燒錄

1. 解壓縮整包，不要直接在 ZIP 裡開程式。
2. STM32CubeIDE → File → Import → General → Existing Projects into Workspace。
3. 選解壓後的 STM32_UART_Workbench 資料夾，勾選專案並 Finish。
4. 專案名稱是 STM32_UART_Workbench，原 Mini_RTOS 可以留著。
5. Build → Debug → 停在 main 時按 F8。新核心 OS_Start 介面已有一起改好，直接用本包全部檔案，不要只貼部分到舊版。

## 2. 確認接收線

USB-to-TTL RX → PA9；TX → PA10；GND 共地。TX/RX 交叉接。這次要有接收，所以 PA10 這條不能漏。使用 3.3 V 邏輯電位。

## 3. 選一個電腦介面

### 桌面介面

在本資料夾開 PowerShell：

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r tools/requirements.txt
.\.venv\Scripts\python.exe tools/workbench_gui.py
```

選 COM → 連線 → 按板上 RESET。統計欄位預設 10 20 30，按「提交」就會回傳 n=3、sum=60、mean=20.000。

### Tera Term

115200 / 8N1 / 無流量控制；Local echo 開啟。輸入 `stats 10 20 30` 並按 Enter。

兩個程式不能同時開同一個 COM 埠。

## 4. 依序試兩個命令

```text
stats 10 20 30
sort 9 -3 4 4 0
```

預期結果：統計平均 20.000；排序 -3 0 4 4 9。

再試 `status`，查看已接受與已完成的工作數。完整指令、限制見 README.md；驗收步驟見 docs/ACCEPTANCE.md；面試說明見 docs/INTERVIEW.md。

## 目前驗證範圍

已完成 PC 自動測試與 ARM Debug/Release 交叉編譯。前一版已在你的板子成功收發；本次移除兩項功能的精簡版仍需重新燒錄驗收；第一次請先貼開機輸出與兩個命令結果。firmware/ 是這次編譯的參考 ELF/BIN，建議先使用 CubeIDE 從原始碼 Build 與燒錄。

## 從前一版更新

1. 停止 CubeIDE Debug，關閉 Python 介面。
2. 將本包的 `Core/Inc/work_job.h`、`Core/Src/work_job.c`、`Core/Src/workbench.c`、`tools/workbench_gui.py`、`tools/send_commands.py` 覆蓋到目前專案對應的位置。這五個檔案需一起更新。
3. CubeIDE 對專案按 Refresh，重新 Build、Debug、Resume；重新開啟 Python 介面並測試 stats、sort、status。
4. 既有 `.venv` 可以沿用，不需重裝套件。接線、COM 設定與轉接器模式也沿用已成功的設定。

你使用的六合一轉接器已確認：黑色 1=ON、2=OFF，銀色選 `232-TTL` 後，Loopback 與前版指令回覆成功。此設定只適用於該款模組，不是所有 USB-to-TTL 的共同開關規格。
