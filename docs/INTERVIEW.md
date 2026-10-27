# 面試時怎麼說明這個應用

## 一句話

我在自製的 STM32 Mini RTOS 上，實作一個可從電腦輸入命令的資料處理終端，透過工作 Queue 讓背景 Task 做統計與排序，並整合 UART 中斷接收、DMA 輸出與 Task 同步。

## 一條流程說完

電腦輸入 → UART RX 中斷將字元放進 ring buffer → RX Semaphore 通知 Console → Console 解析後把整份工作複製進 Queue → Worker 取出計算 → 取得 UART Mutex → 啟動 TX DMA → 等 TX-done Semaphore → 完成後釋放 Mutex。

Console 也要回覆 help/status/錯誤，因此它與 Worker 都會使用 UART；Mutex 在這裡確實保護共享資源，不是獨立展示。兩個 Semaphore 分別處理「有接收字元」和「傳送完成」；Queue 保存的是工作資料。

## 實際展示

輸入 stats 10 20 30，得到最小值、最大值、總和與平均；輸入 sort 9 -3 4 4 0，得到 -3 0 4 4 9。最後查 status，看已接受與完成數。用工作編號說明 Console 提交工作、Worker 回傳結果的關係。

統計和排序資料量小，通常很快結束。這裡展示的是任務分工、資料傳遞與事件同步；沒有用長計算展示忙碌狀態。Queue 滿時程式會拒絕新工作並回 BUSY；這個分支已做 PC 測試，實機滿載另外驗證。

## 這次自己實作的部分

兩個 Task 的分工、命令解析與範圍檢查、資料計算、有限工作 Queue、IRQ 到 Task 的通知、UART 使用權保護與傳輸完成同步、RTOS 啟動與排程時序修正。HAL 負責周邊驅動與初始化；沒有自行設計 DMA 硬體，也沒有使用 FreeRTOS。

## 結果怎麼說

PC 測試與 ARM 編譯可以先驗證；燒錄之後才記錄本版本實機通過的項目。不要把沒有量過的 CPU 使用率、功耗或延遲寫成成果。elapsed_ms 包含計算期間的排程與中斷影響，不是純核心效能。

## 常見追問

- **為什麼 Queue？**命令與計算解耦；Worker 忙時可保留有限數量的後續工作。
- **為什麼還要 Mutex？**Console 與 Worker 都會輸出，且共用格式化緩衝區與 USART1；保護範圍一直到物理傳送完成。
- **為什麼 Semaphore？**把中斷完成事件轉成可阻塞的 Task 等待，事件先到也能保存 token。
- **為什麼 DMA？**減少 CPU 輪詢傳送，不會改變 115200 baud 的線路速度。
- **Queue 滿怎麼辦？**Console 回 BUSY，沒有接受這一筆，讓使用者稍後重送。
- **為什麼不用 float？**核心還沒有完整保存浮點上下文，所以統計使用 64-bit 整數總和與整數格式化平均值。
- **還能改什麼？**傳輸 timeout、多等待者、一般化排程與 Stack 使用量量測。

## 這次實際遇到的問題

開機說明可看到，但電腦送命令後無回覆。先在 UART 接收回呼設中斷點，未觸發；接著把轉接器 TXD/RXD 短接，用 Python 自動送 9 bytes，讀回為空。核對模組文件後，將六合一轉接器設定為 1=ON、2=OFF、232-TTL，Loopback 通過，接回 STM32 後命令回覆成功。這個經驗說明要分段檢查 PC、轉接器、線路、接收中斷與 Task，不能因為收到 BOOT 就假設雙向通訊都正常。
