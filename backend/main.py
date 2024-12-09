from ultralytics import YOLO
import cv2
import numpy as np
from flask import Flask, jsonify
import threading
import sys
from flask_cors import CORS
import time

app = Flask(__name__)
CORS(app)

# Load the YOLOv8 model
model = YOLO('yolov8n.pt')

# Set the desired frame width and height
frame_width = 320
frame_height = 240

vehicle_counts = [0, 0, 0, 0]
stop_flag = False
processing_thread = None

# Add these variables at the top after existing imports
is_processing = False
processing_lock = threading.Lock()

def resize_frame(frame):
    return cv2.resize(frame, (frame_width, frame_height))

# Update cleanup_resources function
def cleanup_resources(caps):
    try:
        for cap in caps:
            if cap and cap.isOpened():
                cap.release()
        cv2.destroyAllWindows()
    except Exception as e:
        print(f"Error during cleanup: {e}")

def detect_vehicles(video_paths):
    global vehicle_counts, stop_flag
    caps = []
    
    try:
        # Initialize video captures
        caps = [cv2.VideoCapture(path) for path in video_paths]
        if not all(cap.isOpened() for cap in caps):
            raise Exception("Error: Couldn't open one or more video streams")

        while not stop_flag:
            # Read and check frames
            frames = []
            for cap in caps:
                ret, frame = cap.read()
                if not ret:
                    stop_flag = True
                    break
                frames.append(resize_frame(frame))

            if stop_flag:
                break

            # Process frames
            top_row = np.hstack((frames[0], frames[1]))
            bottom_row = np.hstack((frames[2], frames[3]))
            large_frame = np.vstack((top_row, bottom_row))

            sub_frames = [
                large_frame[0:frame_height, 0:frame_width],
                large_frame[0:frame_height, frame_width:frame_width*2],
                large_frame[frame_height:frame_height*2, 0:frame_width],
                large_frame[frame_height:frame_height*2, frame_width:frame_width*2]
            ]

            for i, sub_frame in enumerate(sub_frames):
                results = model(sub_frame, conf=0.25, classes=[2, 3, 5, 7])
                vehicle_counts[i] = sum(1 for result in results for box in result.boxes if int(box.cls[0]) in [2, 3, 5, 7])

                # Draw results
                for result in results:
                    for box in result.boxes:
                        x1, y1, x2, y2 = map(int, box.xyxy[0])
                        cv2.rectangle(sub_frame, (x1, y1), (x2, y2), (0, 255, 0), 2)

                cv2.putText(sub_frame, f'Count: {vehicle_counts[i]}', (10, 30), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
                cv2.imshow(f'Video {i+1}', sub_frame)

            # Check for quit command
            if cv2.waitKey(1) & 0xFF == ord('q'):
                stop_flag = True
                break

    except Exception as e:
        print(f"Error: {str(e)}")
        stop_flag = True
    
    finally:
        cleanup_resources(caps)

# Replace the process_video_streams function
def process_video_streams(video_paths):
    global vehicle_counts, stop_flag, is_processing
    caps = []
    
    try:
        # Initialize video captures
        caps = [cv2.VideoCapture(path) for path in video_paths]
        if not all(cap.isOpened() for cap in caps):
            raise Exception("Error: Couldn't open one or more video streams")

        with processing_lock:
            is_processing = True
        
        while not stop_flag:
            # Process each camera feed
            for i, cap in enumerate(caps):
                ret, frame = cap.read()
                if not ret:
                    # If video ends, loop back to beginning
                    cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
                    continue
                
                frame = resize_frame(frame)
                results = model(frame, conf=0.25, classes=[2, 3, 5, 7])
                
                # Update vehicle count
                vehicle_counts[i] = sum(1 for result in results for box in result.boxes 
                                     if int(box.cls[0]) in [2, 3, 5, 7])
                
                # Optional: Show processing window
                annotated_frame = results[0].plot()
                cv2.imshow(f'Camera {i+1}', annotated_frame)
                
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
                
    except Exception as e:
        print(f"Processing error: {str(e)}")
    finally:
        cleanup_resources(caps)
        with processing_lock:
            is_processing = False

def initialize_cameras():
    caps = []
    # Initialize 4 video captures (modify source as needed)
    for i in range(4):
        cap = cv2.VideoCapture(i)  # Use camera index or video file path
        if not cap.isOpened():
            raise Exception(f"Cannot open camera {i}")
        caps.append(cap)
    return caps

# Update the start endpoint
@app.route('/start', methods=['GET'])
def start_processing():
    global processing_thread, stop_flag, is_processing
    
    with processing_lock:
        if is_processing:
            return jsonify({"message": "Processing already running"}), 400
    
    try:
        stop_flag = False
        video_paths = ["video/traffic1.mp4", "video/traffic2.mp4", 
                      "video/traffic3.mp4", "video/traffic4.mp4"]
        
        processing_thread = threading.Thread(
            target=process_video_streams, 
            args=(video_paths,),
            daemon=True
        )
        processing_thread.start()
        
        return jsonify({"message": "Processing started successfully"})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# Replace the stop endpoint
@app.route('/stop', methods=['GET'])
def stop_processing():
    global stop_flag, is_processing, processing_thread
    
    try:
        # Set stop flag
        stop_flag = True
        
        # Wait for processing to stop
        start_time = time.time()
        while is_processing and time.time() - start_time < 5:  # 5 second timeout
            time.sleep(0.1)
        
        # Force cleanup if thread exists
        if processing_thread and processing_thread.is_alive():
            time.sleep(1)  # Give extra time for cleanup
            
        with processing_lock:
            is_processing = False
            
        return jsonify({'status': 'stopped successfully'})
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/traffic<int:id>', methods=['GET'])
def get_traffic(id):
    if 1 <= id <= 4:
        return jsonify({'vehicle_count': vehicle_counts[id-1]})
    return jsonify({'error': 'Invalid traffic ID'}), 400

def start_flask():
    app.run(host='0.0.0.0', port=5000)

# Update main block
if __name__ == '__main__':
    try:
        # Only start Flask server
        app.run(host='0.0.0.0', port=5000)
    except KeyboardInterrupt:
        print("\nShutting down...")
        stop_flag = True
        sys.exit(0)