# 精簡版驗證紀錄

## 本次修改

保留 stats、sort、help、status。移除兩項額外運算功能、命令解析與 GUI 欄位，也移除僅用於長運算的進度回呼。RTOS、Queue、UART、Mutex、Semaphore 的實作未修改。

## 已完成

- Host 演算法與解析、同步與 UART mock、命令到工作結果三組測試。
- 100 組隨機整數陣列與 Python 排序、總和比對。
- 邊界：int32 上下限、64-bit 總和、負平均值、16 個輸入上限；未知與已移除命令拒絕。
- 命令流程：Queue 滿載拒絕與 FIFO、CR/LF/CRLF、過長行與 RX 遺失後復原。
- 同步：早到與晚到 Semaphore 通知、Mutex 交接、Queue 環繞與喚醒、DMA 啟動錯誤、RX ring 滿載。
- ARM GCC 13.2.1 Debug (-O0) 與 Release (-O2) 交叉編譯連結通過。
- Python 工具語法編譯檢查。
- 原廠 FLASH 擴充驅動在 -Wextra 下有三個既有 Banks 參數未使用警告。

## 實機紀錄與待驗收

使用者已確認前一版在 STM32F407 的 BOOT、help、指令與結果回傳成功。曾遇到轉接器模式造成無回傳，設定 1=ON、2=OFF、232-TTL 後 Loopback 與應用回覆成功。

本次精簡版尚需使用者重新 Build、燒錄並確認 stats、sort、status。PC mock 不模擬真實 Context Switch、NVIC、WFI 或 UART 電氣行為。長時間壓力、實機 Queue 滿載、Stack High-Water Mark、功耗與故障恢復尚未完成。
