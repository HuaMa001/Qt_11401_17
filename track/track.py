# track_qt_with_view.py
import argparse
import cv2
import torch
import csv
import os
import sys


def track_video(video_path, output_csv, show=False):
    if not os.path.exists(video_path):
        print(f"Error: 影片不存在: {video_path}")
        return

    cap = cv2.VideoCapture(video_path)
    if not cap.isOpened():
        print(f"Error: 無法開啟影片: {video_path}")
        return

    fps = cap.get(cv2.CAP_PROP_FPS)
    # 取得影片原始尺寸，用於確保座標不越界
    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

    # -----------------------------
    # 載入 YOLOv5 模型 (優先使用 GPU，若無則用 CPU)
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    model = torch.hub.load('ultralytics/yolov5', 'yolov5s', pretrained=True).to(device)
    target_class = 'person'

    # -----------------------------
    # 初始化 CSV
    with open(output_csv, mode='w', newline='') as csv_file:
        csv_writer = csv.writer(csv_file)
        # 欄位保持與 Qt 端一致
        csv_writer.writerow(['time_sec', 'x', 'y', 'w', 'h'])

        frame_idx = 0
        while True:
            ret, frame = cap.read()
            if not ret:
                break

            frame_idx += 1
            time_sec = frame_idx / fps

            # -----------------------------
            # 每幀偵測邏輯：解決 CSRT 漂移問題
            # -----------------------------
            results = model(frame)
            detections = results.pandas().xyxy[0]
            persons = detections[detections['name'] == target_class]

            if len(persons) > 0:
                # 策略：選取置信度 (confidence) 最高的人，或您可以改為選取面積最大的
                person = persons.sort_values(by="confidence", ascending=False).iloc[0]

                x1, y1, x2, y2 = int(person['xmin']), int(person['ymin']), int(person['xmax']), int(person['ymax'])
                w, h = x2 - x1, y2 - y1

                # 計算中心點座標 (這對 Qt 端的置中平移效果最好)
                center_x = (x1 + x2) // 2
                center_y = (y1 + y2) // 2

                # 寫入 CSV
                csv_writer.writerow([round(time_sec, 3), center_x, center_y, w, h])

                # 顯示追蹤框 (僅在 show=True 時執行)
                if show:
                    cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                    cv2.circle(frame, (center_x, center_y), 5, (0, 0, 255), -1)
                    cv2.putText(frame, f"Tracking Person: {round(person['confidence'], 2)}",
                                (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
            else:
                # 如果該幀沒偵測到人，可選擇留空或紀錄上一次位置
                # 這裡選擇不寫入，Qt 端會維持在最後一個已知點
                pass

            if show:
                cv2.imshow("Detection Tracking", frame)
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break

    cap.release()
    if show:
        cv2.destroyAllWindows()
    print(f"Tracking finished. Data saved to {output_csv}")


def main():
    parser = argparse.ArgumentParser(description="改良版 YOLO 每幀偵測追蹤器")
    parser.add_argument("--input", required=True, help="輸入影片路徑")
    parser.add_argument("--output", required=True, help="輸出 CSV 路徑")
    parser.add_argument("--show", action="store_true", help="是否顯示預覽畫面")
    args = parser.parse_args()

    track_video(args.input, args.output, args.show)


if __name__ == "__main__":
    main()
    sys.exit(0)
