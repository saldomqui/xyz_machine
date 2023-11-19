const POINTER_CROSS_SIZE = 20;
const POINTER_CROSS_HALF_SIZE = POINTER_CROSS_SIZE / 2;

const STAND_BY_MODE = 0;
const CALIBRATION_MODE = 1;
const SCANNER_MODE = 2;
const MEASURE_COTES_OFFSET = 40;
const MEASURE_ARROW_LENGTH = 10;
const MEASURE_TEXT_OFFSET = 10;

const MACHINE_X_AXIS_RANGE_MM = 279.89;
const MACHINE_Y_AXIS_RANGE_MM = 187.99;

const MIN_ZOOM = 0.1
const MAX_ZOOM = 1.3

let canvas = 0;
var canvas_ctx = 0;
let scanner_canvas = 0;
let scanner_canvas_ctx = 0;
let scanner_canvas_background = 0;
let scanner_canvas_background_ctx = 0;
let mousex = 0, mousey = 0;
let mode = STAND_BY_MODE;
let calibration_points = [];
let px_per_mm_coef = 0.0;
let last_roi_pos = { x: 0, y: 0 };
let zoom_scale = 1.0;
let last_canvas_scroll_val = 0;

/*
onwheel = (event) => {
    console.log("wheel event");
};*/

function showCalibrationInterface() {
    logStatus("Click on \"Calibrate\" button for starting calibration, then click on two points in the image between which you know the distance");
    console.log("showing calibration interface");
    document.getElementById('container_id').style.display = "none";
    document.getElementById('calibration_section_id').style.display = "block";
}

function showScannerInterface() {
    if (px_per_mm_coef === 0.0) {
        alert("You must perform calibration first");
        return;
    }

    logStatus("Showing the scanner interface");
    document.getElementById('container_id').style.display = "none";
    document.getElementById('scan_section_id').style.display = "block";

    if (scanner_canvas_ctx)
        scanner_canvas_ctx.stroke();
    mode = SCANNER_MODE;
}

function CloseCalibrationInterface() {
    logStatus("Choose one of the options");
    document.getElementById('container_id').style.display = "block";
    document.getElementById('calibration_section_id').style.display = "none";
    mode = STAND_BY_MODE;
}

function CloseScannerInterface() {
    logStatus("Choose one of the options");
    document.getElementById('container_id').style.display = "block";
    document.getElementById('scan_section_id').style.display = "none";
    mode = STAND_BY_MODE;
}

function Calibrate() {
    logStatus("Calibrating... Click first point on the image");
    calibration_points = [];
    px_per_mm_coef = 0.0;
    document.getElementById('calibration_coef_id').value = px_per_mm_coef;
    mode = CALIBRATION_MODE;
}

function ApplyCalibrationDistanceDialog() {
    document.getElementById('calibration_dist_dialog').style.display = "none";
    let calibration_distance = document.getElementById('calibration_distance_id').value;

    //console.log("Computing calibration distance. Num cal pts:" + calibration_points.length + " cal_dist:" + calibration_distance);

    if (calibration_points.length == 2) {
        let px_dist = Math.sqrt(Math.pow(calibration_points[1][1] - calibration_points[0][1], 2.0) + Math.pow(calibration_points[1][0] - calibration_points[0][0], 2.0))
        px_per_mm_coef = px_dist / calibration_distance;
        document.getElementById('calibration_coef_id').value = px_per_mm_coef.toFixed(4);
        logStatus("Calibration: pixel per millimeter = " + px_per_mm_coef);

        //Create scanner canvas with dimensions (MACHING_X_RANGE + IMG_WIDTH, MACHING_Y_RANGE + IMG_HEIGHT)

        scanner_canvas = document.getElementById('scan_canvas_id');

        //canvas.height = (canvas.width * this.height) / this.width;
        scanner_canvas.width = MACHINE_X_AXIS_RANGE_MM * px_per_mm_coef + canvas.width;
        scanner_canvas.height = MACHINE_Y_AXIS_RANGE_MM * px_per_mm_coef + canvas.height;

        //console.log("Scanner Canvas div width:" + scanner_canvas.width + " height:" + scanner_canvas.height);

        scanner_canvas_ctx = scanner_canvas.getContext('2d');

        scanner_canvas_ctx.strokeStyle = '#0000ff';
        scanner_canvas_ctx.fillStyle = '#000000ff';
        scanner_canvas_ctx.lineWidth = 10;
        scanner_canvas_ctx.beginPath();
        scanner_canvas_ctx.fillRect(0, 0, scanner_canvas.width, scanner_canvas.height);
        scanner_canvas_ctx.rect(canvas.width / 2, canvas.height / 2, scanner_canvas.width - canvas.width, scanner_canvas.height - canvas.height);
        scanner_canvas_ctx.closePath();
        scanner_canvas_ctx.stroke();
        //scanner_canvas_ctx.font = "20px serif";

        scanner_canvas_background = document.getElementById('scan_canvas_background_id');
        scanner_canvas_background.width = scanner_canvas.width;
        scanner_canvas_background.height = scanner_canvas.height;
        scanner_canvas_background_ctx = scanner_canvas_background.getContext('2d');
        scanner_canvas_background_ctx.drawImage(scanner_canvas, 0, 0);
        scanner_canvas_background_ctx.stroke();

        document.getElementById('scan_canvas_id').addEventListener("wheel", (event) => {
            //console.log(event);
            //console.log("event listener: wheel event. deltaX:" + event.deltaX + " deltaY:" + event.deltaY);
            let delta_zoom;
            let new_zoom;
            let zoom_ratio;
            if (event.deltaY > 0) {
                delta_zoom = -0.05;
                new_zoom = zoom_scale + delta_zoom;

                if (new_zoom >= MIN_ZOOM) {
                    zoom_ratio = new_zoom / zoom_scale;
                    zoom_scale = new_zoom;
                    //console.log("zoom scale:" + zoom_scale + "canvas width:" + scanner_canvas_ctx.canvas.width + " canvas height:" + scanner_canvas_ctx.canvas.height);
                    scanner_canvas_ctx.scale(zoom_ratio, zoom_ratio);
                    scanner_canvas_ctx.canvas.width *= zoom_ratio;
                    scanner_canvas_ctx.canvas.height *= zoom_ratio;
                    scanner_canvas_ctx.drawImage(scanner_canvas_background, 0, 0, scanner_canvas_background.width, scanner_canvas_background.height, 0, 0, scanner_canvas.width, scanner_canvas.height);

                    computeNewScroll(event.x, event.y, zoom_ratio);
                }
            } else {
                delta_zoom = 0.05;
                new_zoom = zoom_scale + delta_zoom;

                if (new_zoom <= MAX_ZOOM) {
                    zoom_ratio = new_zoom / zoom_scale;
                    zoom_scale = new_zoom;
                    //console.log("zoom scale:" + zoom_scale + "canvas width:" + scanner_canvas_ctx.canvas.width + " canvas height:" + scanner_canvas_ctx.canvas.height);
                    scanner_canvas_ctx.scale(zoom_ratio, zoom_ratio);
                    scanner_canvas_ctx.canvas.width *= zoom_ratio;
                    scanner_canvas_ctx.canvas.height *= zoom_ratio;
                    scanner_canvas_ctx.drawImage(scanner_canvas_background, 0, 0, scanner_canvas_background.width, scanner_canvas_background.height, 0, 0, scanner_canvas.width, scanner_canvas.height);
                    //document.getElementById('scan_canvas_frame_id').scrollTop *= (1.0 + delta_zoom);
                    computeNewScroll(event.x, event.y, zoom_ratio);
                }
            }
        });

    }

    mode = STAND_BY_MODE;
}

function mouse(e) {
    mousex = e.clientX;
    mousey = e.clientY;
}

function scan_canvas_click(e) {
    let elem = document.getElementById('scan_canvas_frame_id');

    //console.log("Canvas frame width:"+elem.offsetWidth + " mouse x:" + mx + " scroll:"+elem.scrollLeft);

    let x_px = (e.clientX - scanner_canvas.offsetLeft + elem.scrollLeft) / zoom_scale;
    let y_px = (e.clientY - scanner_canvas.offsetTop + elem.scrollTop) / zoom_scale;
    let xy_mm = getMachineXYFromPxCoods(x_px, y_px);

    console.log("Zoom scale:" + zoom_scale + " Clicked scan canvas at x:" + xy_mm[0] + " y:" + xy_mm[1]);
    setMachineXYPos(xy_mm[0], xy_mm[1]);
}

function zoom(e) {
    console.log(e);
}
/*
function scroll_off(e) {
    let div_elem = document.getElementById('scan_canvas_frame_id');
    div_elem.style.overflow = "hidden";
    console.log("disabling scroll on scan_canvas");
}
function scroll_on(e) {
    document.getElementById('scan_canvas_frame_id').style.overflow = "auto";
    console.log("enabling scroll on scan_canvas");
}*/

function canvas_click(e) {
    if (mode == CALIBRATION_MODE) {
        let canvas_pointer_x = mousex - canvas.offsetLeft,
            canvas_pointer_y = mousey - canvas.offsetTop;

        console.log("Clicked: X:" + canvas_pointer_x + " Y:" + canvas_pointer_y);
        calibration_points.push([canvas_pointer_x, canvas_pointer_y]);

        if (calibration_points.length == 1) {
            logStatus("Click second point on the image");
        } else if (calibration_points.length == 2) {
            logStatus("Calibration finished");

            document.getElementById('calibration_dist_dialog').style.display = "block";
        }
    }
}

function setup() {
    console.log("setting up");

    cam_img_raw = document.getElementById('cam_img_raw_id');
    cam_img_raw.src = "http://" + window.location.host + ":8888?quality=100";

    cam_img_raw.onload = function () {
        console.log("input img width:" + this.width + " height:" + this.height);
        canvas = document.getElementById('canvas_id');

        //canvas.height = (canvas.width * this.height) / this.width;
        canvas.width = this.width;
        canvas.height = this.height;

        console.log("Canvas div width:" + canvas.offsetWidth + " height:" + canvas.offsetHeight);

        canvas_ctx = canvas.getContext('2d');
        canvas_ctx.font = "20px serif";
        canvas_ctx.fillStyle = "#00ff00";

        //console.log(canvas_ctx);
    }
}

function draw() {
    if (canvas != 0) {
        let canvas_pointer_x = mousex - canvas.offsetLeft,
            canvas_pointer_y = mousey - canvas.offsetTop;

        canvas_ctx.drawImage(cam_img_raw, 0, 0, canvas.width, canvas.height, 0, 0, cam_img_raw.width, cam_img_raw.height);

        // Draw pointer cross
        canvas_ctx.beginPath();
        canvas_ctx.moveTo(canvas_pointer_x - POINTER_CROSS_HALF_SIZE, canvas_pointer_y);
        canvas_ctx.lineTo(canvas_pointer_x + POINTER_CROSS_HALF_SIZE, canvas_pointer_y);
        canvas_ctx.moveTo(canvas_pointer_x, canvas_pointer_y - POINTER_CROSS_HALF_SIZE);
        canvas_ctx.lineTo(canvas_pointer_x, canvas_pointer_y + POINTER_CROSS_HALF_SIZE);
        canvas_ctx.closePath();

        canvas_ctx.strokeStyle = '#ffff00';
        canvas_ctx.stroke();

        if (px_per_mm_coef != 0.0) {
            canvas_ctx.beginPath();
            canvas_ctx.moveTo(0, canvas.height - MEASURE_COTES_OFFSET);
            canvas_ctx.lineTo(canvas.width - 1, canvas.height - MEASURE_COTES_OFFSET);
            canvas_ctx.moveTo(0, canvas.height - MEASURE_COTES_OFFSET);
            canvas_ctx.lineTo(MEASURE_ARROW_LENGTH, canvas.height - MEASURE_COTES_OFFSET - MEASURE_ARROW_LENGTH);
            canvas_ctx.moveTo(0, canvas.height - MEASURE_COTES_OFFSET);
            canvas_ctx.lineTo(MEASURE_ARROW_LENGTH, canvas.height - MEASURE_COTES_OFFSET + MEASURE_ARROW_LENGTH);
            canvas_ctx.moveTo(canvas.width - 1, canvas.height - MEASURE_COTES_OFFSET);
            canvas_ctx.lineTo(canvas.width - 1 - MEASURE_ARROW_LENGTH, canvas.height - MEASURE_COTES_OFFSET - MEASURE_ARROW_LENGTH);
            canvas_ctx.moveTo(canvas.width - 1, canvas.height - MEASURE_COTES_OFFSET);
            canvas_ctx.lineTo(canvas.width - 1 - MEASURE_ARROW_LENGTH, canvas.height - MEASURE_COTES_OFFSET + MEASURE_ARROW_LENGTH);
            canvas_ctx.closePath();

            let img_width_mm = canvas.width / px_per_mm_coef;
            canvas_ctx.fillText(img_width_mm.toFixed(2) + "[mm]", canvas.width / 2, canvas.height - MEASURE_COTES_OFFSET - MEASURE_TEXT_OFFSET);

            canvas_ctx.moveTo(MEASURE_COTES_OFFSET, 0);
            canvas_ctx.lineTo(MEASURE_COTES_OFFSET, canvas.height - 1);
            canvas_ctx.moveTo(MEASURE_COTES_OFFSET, 0);
            canvas_ctx.lineTo(MEASURE_COTES_OFFSET + MEASURE_ARROW_LENGTH, MEASURE_ARROW_LENGTH);
            canvas_ctx.moveTo(MEASURE_COTES_OFFSET, 0);
            canvas_ctx.lineTo(MEASURE_COTES_OFFSET - MEASURE_ARROW_LENGTH, MEASURE_ARROW_LENGTH);
            canvas_ctx.moveTo(MEASURE_COTES_OFFSET, canvas.height - 1);
            canvas_ctx.lineTo(MEASURE_COTES_OFFSET - MEASURE_ARROW_LENGTH, canvas.height - 1 - MEASURE_ARROW_LENGTH);
            canvas_ctx.moveTo(MEASURE_COTES_OFFSET, canvas.height - 1);
            canvas_ctx.lineTo(MEASURE_COTES_OFFSET + MEASURE_ARROW_LENGTH, canvas.height - 1 - MEASURE_ARROW_LENGTH);

            let img_height_mm = canvas.height / px_per_mm_coef;
            canvas_ctx.fillText(img_height_mm.toFixed(2) + "[mm]", MEASURE_COTES_OFFSET + MEASURE_TEXT_OFFSET, canvas.height / 2);

            canvas_ctx.strokeStyle = '#ffffff';
            canvas_ctx.stroke();

            drawFrameOnScanCanvasAtPos(document.getElementById('x_pos_id').value, document.getElementById('y_pos_id').value);
        } else {
            canvas_ctx.fillText("NOT CALIBRATED", 20, canvas.height - 20);
        }
    }
}

function getPxCoordsFromMachineXY(x_mm, y_mm) {
    let x_px = canvas.width / 2 + x_mm * px_per_mm_coef;
    let y_px = canvas.height / 2 + y_mm * px_per_mm_coef;

    return [x_px, y_px];
}

function getMachineXYFromPxCoods(x_px, y_px) {
    let x_mm = (x_px - (canvas.width / 2)) / px_per_mm_coef;
    let y_mm = (y_px - (canvas.height / 2)) / px_per_mm_coef;

    return [x_mm, y_mm];
}


function drawFrameOnScanCanvasAtPos(x_center, y_center) {
    let center_px = getPxCoordsFromMachineXY(x_center, y_center);
    let top_left_corner_px = [center_px[0] - canvas.width / 2, center_px[1] - canvas.height / 2];

    //console.log("drawing frame on scan img: top:[" + top_left_corner_px[0] + ", " + top_left_corner_px[1] + "], prev:[" + last_roi_pos[0] + ", " + last_roi_pos[1] + "]");

    scanner_canvas_ctx.drawImage(scanner_canvas_background, last_roi_pos[0], last_roi_pos[1], canvas.width, canvas.height, last_roi_pos[0] * zoom_scale, last_roi_pos[1] * zoom_scale, canvas.width * zoom_scale, canvas.height * zoom_scale);
    scanner_canvas_ctx.drawImage(cam_img_raw, 0, 0, canvas.width, canvas.height, top_left_corner_px[0] * zoom_scale, top_left_corner_px[1] * zoom_scale, canvas.width * zoom_scale, canvas.height * zoom_scale);
    scanner_canvas_ctx.lineWidth = 1;
    scanner_canvas_ctx.fillStyle = '#000000ff';
    // Draw pointer cross
    scanner_canvas_ctx.beginPath();
    scanner_canvas_ctx.moveTo((center_px[0] - POINTER_CROSS_HALF_SIZE) * zoom_scale, center_px[1] * zoom_scale);
    scanner_canvas_ctx.lineTo((center_px[0] + POINTER_CROSS_HALF_SIZE) * zoom_scale, center_px[1] * zoom_scale);
    scanner_canvas_ctx.moveTo((center_px[0] * zoom_scale), (center_px[1] - POINTER_CROSS_HALF_SIZE) * zoom_scale);
    scanner_canvas_ctx.lineTo(center_px[0] * zoom_scale, (center_px[1] + POINTER_CROSS_HALF_SIZE) * zoom_scale);
    scanner_canvas_ctx.closePath();
    scanner_canvas_ctx.rect(top_left_corner_px[0] * zoom_scale + 1, top_left_corner_px[1] * zoom_scale + 1, canvas.width * zoom_scale - 2, canvas.height * zoom_scale - 2);
    scanner_canvas_ctx.strokeStyle = '#00ff00';
    scanner_canvas_ctx.stroke();
    scanner_canvas_ctx.setTransform(1, 0, 0, 1, 0, 0);

    last_roi_pos[0] = top_left_corner_px[0];
    last_roi_pos[1] = top_left_corner_px[1];
}

function projectImage() {
    console.log("Projecting current frame on scan image")
    scanner_canvas_background_ctx.drawImage(cam_img_raw, 0, 0, canvas.width, canvas.height, last_roi_pos[0], last_roi_pos[1], canvas.width, canvas.height);
    scanner_canvas_background_ctx.stroke();
}

function computeNewScroll(mx, my, coef) {
    let elem = document.getElementById('scan_canvas_frame_id');

    //console.log("Canvas frame width:"+elem.offsetWidth + " mouse x:" + mx + " scroll:"+elem.scrollLeft);

    let new_center_y = (my + elem.scrollTop) * coef;
    let new_center_x = (mx + elem.scrollLeft) * coef;
    //console.log("Centering at: x:" + new_center_x + " y:" + new_center_y);

    let new_scroll_y = new_center_y - my;
    let new_scroll_x = new_center_x - mx;

    if (new_scroll_y < 0)
        new_scroll_y = 0;
    else if (new_scroll_y > (scanner_canvas.height - elem.clientHeight)) {
        new_scroll_y = (scanner_canvas.height - elem.clientHeight);
    }

    if (new_scroll_x < 0)
        new_scroll_x = 0;
    else if (new_scroll_x > (scanner_canvas.width - elem.clientWidth)) {
        new_scroll_x = (scanner_canvas.width - elem.clientWidth);
    }

    elem.scrollTo(new_scroll_x, new_scroll_y);
    //console.log("prev scroll x:" + curr_scroll_x + " new_scroll_x:" + elem.scrollLeft);
    //console.log("prev scroll y:" + curr_scroll_y + " new_scroll_y:" + elem.scrollTop);
}

function setMachineXYPos(x, y) {
    const msg = {
        x: x,
        y: y
    };
    socket_ws.send("set_xy_pos", msg);
}
