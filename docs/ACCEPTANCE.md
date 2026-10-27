# STM32F407 實機驗收

本文件是待執行的驗收清單，不代表測試已經通過。

## 先確認

- 匯入新專案 STM32_UART_Workbench；原 Mini_RTOS 可保留。
- USB-to-TTL RX 接 PA9；TX 接 PA10；GND 共地，3.3 V 邏輯。
- Tera Term：正確 COM、115200 8N1、無流量控制、Local echo 開啟。
- Build、下載驗證成功；Debug 若停在 main，按 F8。按板上 RESET 後應看到 BOOT、help 與 READY。

## 先做兩個結果測試

1. `stats 10 20 30`：n=3、min=10、max=30、sum=60、mean=20.000。
2. `sort 9 -3 4 4 0`：-3 0 4 4 9。

每個工作先收到 QUEUED #N，再收到相同編號的 RESULT #N；編號在重開機後重設。

## 回應與排隊

- 連續送多筆工作，確認已接受工作的結果維持 FIFO。
- 超過四筆待處理工作的額外命令應回 BUSY，不能覆寫先前工作。可用 Python 工具快速連送。
- `status` 確認 rx_drop=0、rx_err=0、tx_err=0。accepted/completed 隨成功工作增加。
- 工作運算與輸出速度取決於實機；不要僅靠手打慢速命令就宣稱滿載測試已通過。

## 邊界與錯誤

- `stats -1 0 0`：mean=-0.333。
- `stats 2147483647 2147483647`：sum=4294967294。
- `stats 2147483648`、未知命令：ERR，之後仍能正常處理有效命令。
- 貼入超過 255 bytes 的行：ERR，不執行殘缺資料；下一行有效命令正常。
- CR、LF、CRLF 各測一次；CRLF 不應提交兩次。檢查 Backspace 行為。
- 若貼上過快造成 rx_drop 或 rx_err，這一行應被拒絕；放慢速度後重送，不應把殘缺輸入當有效工作。

## 穩定性

- 交替跑 stats / sort 與 status，持續至少數分鐘。
- Suspend 後 Resume，確認仍可接受命令；沒有主動 reset 時，不應反覆出現 BOOT。
- 若出錯，保存最後幾行 UART、Debug 停住位置、os_panic_code、uart_dma_last_status 與各錯誤計數。

BOOT 也沒有：確認 F8、下載檔、COM、供電與初始化；有 BOOT 但沒有 READY：檢查 Task 啟動與 TX DMA 中斷；有 READY 但輸入無反應：先確認轉接器模式與 TX→PA10 接收線；必要時拔開 STM32，先做轉接器 TXD/RXD Loopback。
