"use strict";
const { useEffect, useMemo, useRef, useState } = React;
const SERVICE_UUID = "00001f10-0000-1000-8000-00805f9b34fb";
const CHARACTERISTIC_UUID = "00001f1f-0000-1000-8000-00805f9b34fb";
const DATA_CHARACTERISTIC_UUID = "00001f11-0000-1000-8000-00805f9b34fb";
const DISPLAY_WIDTH = 212;
const DISPLAY_HEIGHT = 104;
const PANEL_BUFFER_WIDTH = 104;
const PANEL_BUFFER_HEIGHT = 212;
const BUFFER_SIZE = (PANEL_BUFFER_WIDTH * PANEL_BUFFER_HEIGHT) / 8;
const CMD_CLEAR_DISPLAY = 0x00;
const CMD_DISPLAY_IMAGE = 0x01;
const CMD_WRITE_DATA = 0x03;
const CMD_SAVE_IMAGE = 0x04;
const CMD_SET_IMAGE_SIZE = 0x06;
const initialImageParams = {
    brightness: 100,
    contrast: 100,
    dithering: "none",
    ditherIntensity: 1,
    mtu: 20,
    confirmInterval: 20,
};
const initialFabricFields = [
    { id: "fabricWidth", label: "Khổ vải", value: "62/66 NEW" },
    { id: "fabricStaff", label: "Nhân viên xả", value: "Tai" },
    { id: "fabricPo", label: "PO", value: "302J07" },
    { id: "fabricRelaxDate", label: "Ngày xả", value: "12/3 9h" },
    { id: "fabricOkDate", label: "Ngày OK", value: "14/3" },
    { id: "fabricItem", label: "Item", value: "Tie: 0464" },
    { id: "fabricColor", label: "Màu", value: "Caviar" },
    { id: "fabricLot", label: "Lot", value: "4/296 6/985" },
    { id: "fabricBuy", label: "Buy", value: "1224" },
    { id: "fabricRoll", label: "Roll", value: "101" },
    { id: "fabricYds", label: "YDS", value: "489" },
    { id: "fabricNote", label: "Ghi chú", value: "T4" },
];
const screenModes = [
    {
        key: "clock",
        label: "Đồng hồ",
        description: "Đồng hồ thời gian",
        modeValue: 0,
    },
    { key: "calendar", label: "Lịch", description: "Lịch cơ bản", modeValue: 1 },
    {
        key: "calendarAnalog",
        label: "Lịch & đồng hồ",
        description: "Lịch và đồng hồ kim",
        modeValue: 2,
    },
    {
        key: "image",
        label: "Hình ảnh",
        description: "Hiển thị ảnh đơn sắc",
        modeValue: 3,
    },
    { key: "fabric", label: "Thẻ kho", description: "Thẻ kho vải", modeValue: 4 },
];
function delay(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
}
function formatCurrentTime(date) {
    const year = date.getFullYear();
    const month = String(date.getMonth() + 1).padStart(2, "0");
    const day = String(date.getDate()).padStart(2, "0");
    const hours = String(date.getHours()).padStart(2, "0");
    const minutes = String(date.getMinutes()).padStart(2, "0");
    const seconds = String(date.getSeconds()).padStart(2, "0");
    return `${year}-${month}-${day} ${hours}:${minutes}:${seconds}`;
}
function imageDataToString(epdData) {
    let str = "";
    for (let i = 0; i < epdData.length; i += 1) {
        str += `0x${epdData[i].toString(16).padStart(2, "0")}`;
        if (i < epdData.length - 1) {
            str += ",";
            str += (i + 1) % 16 === 0 ? "\n" : " ";
        }
    }
    return str;
}
function stringToImageData(str) {
    const cleanStr = str.replace(/[\[\]{}]/g, "");
    const matches = cleanStr.match(/0[xX][0-9a-fA-F]{2}/g) || [];
    if (matches.length === 0) {
        throw new Error("Không tìm thấy dữ liệu 0xXX hợp lệ");
    }
    const result = new Uint8Array(matches.length);
    for (let i = 0; i < matches.length; i += 1) {
        result[i] = parseInt(matches[i].slice(2), 16);
    }
    return result;
}
function resizeImage(image, width, height) {
    const canvas = document.createElement("canvas");
    canvas.width = width;
    canvas.height = height;
    const ctx = canvas.getContext("2d");
    if (!ctx) {
        throw new Error("Không tạo được canvas context");
    }
    ctx.fillStyle = "white";
    ctx.fillRect(0, 0, width, height);
    const shouldRotate = image.height > image.width;
    const sourceWidth = shouldRotate ? image.height : image.width;
    const sourceHeight = shouldRotate ? image.width : image.height;
    const aspectRatio = sourceWidth / sourceHeight;
    const isSquareLike = aspectRatio > 0.8 && aspectRatio < 1.25;
    const scale = isSquareLike
        ? Math.min(width / sourceWidth, height / sourceHeight) * 0.92
        : Math.max(width / sourceWidth, height / sourceHeight);
    const drawWidth = Math.round(sourceWidth * scale);
    const drawHeight = Math.round(sourceHeight * scale);
    const offsetX = Math.floor((width - drawWidth) / 2);
    const offsetY = Math.floor((height - drawHeight) / 2);
    ctx.save();
    if (shouldRotate) {
        ctx.translate(width / 2, height / 2);
        ctx.rotate(-Math.PI / 2);
        ctx.drawImage(image, -drawHeight / 2, -drawWidth / 2, drawHeight, drawWidth);
    }
    else {
        ctx.drawImage(image, offsetX, offsetY, drawWidth, drawHeight);
    }
    ctx.restore();
    return ctx.getImageData(0, 0, width, height);
}
function applyDithering(imageData, ditherType, intensity) {
    if (ditherType === "none") {
        return imageData;
    }
    const width = imageData.width;
    const height = imageData.height;
    const output = new Uint8Array(imageData.data);
    if (ditherType === "floyd-steinberg") {
        for (let y = 0; y < height; y += 1) {
            for (let x = 0; x < width; x += 1) {
                const index = (y * width + x) * 4;
                const oldGray = Math.round(0.299 * output[index] +
                    0.587 * output[index + 1] +
                    0.114 * output[index + 2]);
                const newGray = oldGray < 128 ? 0 : 255;
                const error = (oldGray - newGray) * intensity;
                output[index] = newGray;
                output[index + 1] = newGray;
                output[index + 2] = newGray;
                const errorSpread = [
                    { x: 1, y: 0, factor: 7 / 16 },
                    { x: -1, y: 1, factor: 3 / 16 },
                    { x: 0, y: 1, factor: 5 / 16 },
                    { x: 1, y: 1, factor: 1 / 16 },
                ];
                for (const spread of errorSpread) {
                    const nx = x + spread.x;
                    const ny = y + spread.y;
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        const nIndex = (ny * width + nx) * 4;
                        output[nIndex] = Math.max(0, Math.min(255, output[nIndex] + error * spread.factor));
                        output[nIndex + 1] = Math.max(0, Math.min(255, output[nIndex + 1] + error * spread.factor));
                        output[nIndex + 2] = Math.max(0, Math.min(255, output[nIndex + 2] + error * spread.factor));
                    }
                }
            }
        }
    }
    else {
        const ditherMatrix = [
            [0, 8, 2, 10],
            [12, 4, 14, 6],
            [3, 11, 1, 9],
            [15, 7, 13, 5],
        ];
        const matrixSize = 4;
        for (let y = 0; y < height; y += 1) {
            for (let x = 0; x < width; x += 1) {
                const index = (y * width + x) * 4;
                const oldGray = Math.round(0.299 * output[index] +
                    0.587 * output[index + 1] +
                    0.114 * output[index + 2]);
                const threshold = (ditherMatrix[y % matrixSize][x % matrixSize] /
                    (matrixSize * matrixSize)) *
                    255 *
                    intensity;
                const newGray = oldGray < threshold ? 0 : 255;
                output[index] = newGray;
                output[index + 1] = newGray;
                output[index + 2] = newGray;
            }
        }
    }
    return new ImageData(new Uint8ClampedArray(output), width, height);
}
function convertImageToEPDData(imageData, imageParams) {
    const tempCanvas = document.createElement("canvas");
    tempCanvas.width = DISPLAY_WIDTH;
    tempCanvas.height = DISPLAY_HEIGHT;
    const tempCtx = tempCanvas.getContext("2d");
    if (!tempCtx) {
        throw new Error("Không tạo được canvas context");
    }
    tempCtx.putImageData(imageData, 0, 0);
    const data = tempCtx.getImageData(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    const pixelData = new Uint8Array(data.data);
    const brightnessFactor = imageParams.brightness / 100;
    const contrastFactor = imageParams.contrast / 100;
    for (let i = 0; i < pixelData.length; i += 4) {
        let r = pixelData[i];
        let g = pixelData[i + 1];
        let b = pixelData[i + 2];
        r = Math.round((r - 128) * contrastFactor + 128) * brightnessFactor;
        g = Math.round((g - 128) * contrastFactor + 128) * brightnessFactor;
        b = Math.round((b - 128) * contrastFactor + 128) * brightnessFactor;
        pixelData[i] = Math.max(0, Math.min(255, r));
        pixelData[i + 1] = Math.max(0, Math.min(255, g));
        pixelData[i + 2] = Math.max(0, Math.min(255, b));
    }
    data.data.set(pixelData);
    tempCtx.putImageData(data, 0, 0);
    const ditheredImageData = applyDithering(data, imageParams.dithering, imageParams.ditherIntensity);
    const epdData = new Uint8Array(BUFFER_SIZE);
    epdData.fill(0xff);
    const widthByte = PANEL_BUFFER_WIDTH / 8;
    const ditheredData = ditheredImageData.data;
    for (let x = 0; x < DISPLAY_WIDTH; x += 1) {
        for (let y = 0; y < DISPLAY_HEIGHT; y += 1) {
            const index = (y * DISPLAY_WIDTH + x) * 4;
            if (ditheredData[index] >= 128) {
                continue;
            }
            const rawX = y;
            const rawY = x;
            const epdIndex = Math.floor(rawX / 8) + rawY * widthByte;
            epdData[epdIndex] &= ~(0x80 >> (rawX % 8));
        }
    }
    return epdData;
}
function App() {
    const previewRef = useRef(null);
    const deviceRef = useRef(null);
    const controlCharacteristicRef = useRef(null);
    const dataCharacteristicRef = useRef(null);
    const rotationTimerRef = useRef(null);
    const rotationIndexRef = useRef(0);
    const [workspaceMode, setWorkspaceMode] = useState("mode-1");
    const [sidebarOpen, setSidebarOpen] = useState(false);
    const [locale, setLocale] = useState("vi");
    const [theme, setTheme] = useState("light");
    const [currentTime, setCurrentTime] = useState(() => formatCurrentTime(new Date()));
    const [deviceName, setDeviceName] = useState("Chưa kết nối");
    const [connected, setConnected] = useState(false);
    const [connecting, setConnecting] = useState(false);
    const [statusLines, setStatusLines] = useState([
        "Hệ thống sẵn sàng.",
    ]);
    const [progress, setProgress] = useState(0);
    const [imageFile, setImageFile] = useState(null);
    const [currentScreenMode, setCurrentScreenMode] = useState("clock");
    const [imageParams, setImageParams] = useState(initialImageParams);
    const [fabricFields, setFabricFields] = useState(initialFabricFields);
    const [fabricModalOpen, setFabricModalOpen] = useState(false);
    const [rotationSelection, setRotationSelection] = useState([
        "clock",
        "calendar",
        "image",
    ]);
    const [rotationIntervalSec, setRotationIntervalSec] = useState(15);
    const [rotationEnabled, setRotationEnabled] = useState(false);
    const [loadingState, setLoadingState] = useState({
        active: false,
        title: "",
        detail: "",
    });
    const bluetoothAvailable = useMemo(() => typeof navigator !== "undefined" && "bluetooth" in navigator, []);
    const currentModeConfig = useMemo(() => screenModes.find((mode) => mode.key === currentScreenMode) ||
        screenModes[0], [currentScreenMode]);
    const getScreenModeLabel = (mode) => {
        const labels = {
            image: locale === "vi" ? "Hình ảnh" : "Image",
            calendar: locale === "vi" ? "Lịch" : "Calendar",
            clock: locale === "vi" ? "Đồng hồ" : "Clock",
            calendarAnalog: locale === "vi" ? "Lịch & đồng hồ" : "Calendar & clock",
            fabric: locale === "vi" ? "Thẻ kho" : "Fabric card",
        };
        return labels[mode];
    };
    const appendStatus = (message) => {
        setStatusLines((prev) => [
            ...prev,
            `[${new Date().toLocaleTimeString("vi-VN")}] ${message}`,
        ]);
        console.log(message);
    };
    const setLoading = (title, detail) => {
        setLoadingState({ active: true, title, detail });
    };
    const clearLoading = () => {
        setLoadingState({ active: false, title: "", detail: "" });
    };
    const runBusyTask = async (title, detail, task) => {
        setLoading(title, detail);
        try {
            await task();
        }
        finally {
            clearLoading();
        }
    };
    const stopRotation = (silent) => {
        if (rotationTimerRef.current) {
            window.clearInterval(rotationTimerRef.current);
            rotationTimerRef.current = null;
        }
        if (rotationEnabled) {
            setRotationEnabled(false);
            if (!silent) {
                appendStatus("Đã dừng lịch tự động chuyển màn hình.");
            }
        }
    };
    const resetConnection = () => {
        controlCharacteristicRef.current = null;
        dataCharacteristicRef.current = null;
        deviceRef.current = null;
        setDeviceName("Chưa kết nối");
        setConnected(false);
        stopRotation(true);
    };
    useEffect(() => {
        document.body.dataset.theme = theme;
    }, [theme]);
    useEffect(() => {
        const timer = window.setInterval(() => {
            setCurrentTime(formatCurrentTime(new Date()));
        }, 1000);
        return () => window.clearInterval(timer);
    }, []);
    useEffect(() => {
        return () => {
            stopRotation(true);
            const device = deviceRef.current;
            if (device?.gatt?.connected) {
                device.gatt.disconnect();
            }
        };
    }, []);
    useEffect(() => {
        const canvas = previewRef.current;
        if (!canvas) {
            return;
        }
        const ctx = canvas.getContext("2d");
        if (!ctx) {
            return;
        }
        let active = true;
        const drawPreview = async () => {
            if (!imageFile) {
                ctx.fillStyle = "white";
                ctx.fillRect(0, 0, canvas.width, canvas.height);
                return;
            }
            try {
                const image = await createImageBitmap(imageFile);
                if (!active) {
                    return;
                }
                const resized = resizeImage(image, DISPLAY_WIDTH, DISPLAY_HEIGHT);
                const processed = convertImageToEPDData(resized, imageParams);
                const previewImageData = new ImageData(DISPLAY_WIDTH, DISPLAY_HEIGHT);
                const widthByte = PANEL_BUFFER_WIDTH / 8;
                for (let x = 0; x < DISPLAY_WIDTH; x += 1) {
                    for (let y = 0; y < DISPLAY_HEIGHT; y += 1) {
                        const rawX = y;
                        const rawY = x;
                        const epdIndex = Math.floor(rawX / 8) + rawY * widthByte;
                        const isBlack = (processed[epdIndex] & (0x80 >> (rawX % 8))) === 0;
                        const pixelIndex = (y * DISPLAY_WIDTH + x) * 4;
                        const color = isBlack ? 0 : 255;
                        previewImageData.data[pixelIndex] = color;
                        previewImageData.data[pixelIndex + 1] = color;
                        previewImageData.data[pixelIndex + 2] = color;
                        previewImageData.data[pixelIndex + 3] = 255;
                    }
                }
                ctx.putImageData(previewImageData, 0, 0);
            }
            catch (error) {
                console.error(error);
            }
        };
        void drawPreview();
        return () => {
            active = false;
        };
    }, [imageFile, imageParams]);
    const ensureControlCharacteristic = () => {
        const characteristic = controlCharacteristicRef.current;
        if (!characteristic) {
            throw new Error("Vui lòng kết nối thiết bị trước");
        }
        return characteristic;
    };
    const ensureDataCharacteristic = () => {
        const characteristic = dataCharacteristicRef.current;
        if (!characteristic) {
            throw new Error("Chưa có kênh dữ liệu hình ảnh");
        }
        return characteristic;
    };
    const sendCommand = async (command, data = []) => {
        const characteristic = ensureControlCharacteristic();
        const buffer = new Uint8Array([command, ...data]);
        appendStatus(`Gui lenh: ${Array.from(buffer)
            .map((b) => b.toString(16).padStart(2, "0"))
            .join(" ")}`);
        await characteristic.writeValue(buffer);
    };
    const sendDataCommand = async (command, data = []) => {
        const characteristic = ensureDataCharacteristic();
        const buffer = new Uint8Array([command, ...data]);
        appendStatus(`Gui du lieu: ${Array.from(buffer)
            .map((b) => b.toString(16).padStart(2, "0"))
            .join(" ")}`);
        await characteristic.writeValue(buffer);
    };
    const sendTimestamp = async (unixTime) => {
        const characteristic = ensureControlCharacteristic();
        const buffer = new Uint8Array(5);
        buffer[0] = 0xdd;
        buffer[1] = (unixTime >> 24) & 0xff;
        buffer[2] = (unixTime >> 16) & 0xff;
        buffer[3] = (unixTime >> 8) & 0xff;
        buffer[4] = unixTime & 0xff;
        await characteristic.writeValue(buffer);
    };
    const sendScreenMode = async (modeKey) => {
        const mode = screenModes.find((item) => item.key === modeKey);
        if (!mode) {
            return;
        }
        await sendCommand(0xe1, [mode.modeValue]);
        await delay(200);
        await sendCommand(0xe2);
        setCurrentScreenMode(mode.key);
        appendStatus(`Đã chuyển sang màn hình ${mode.label}.`);
    };
    const prepareImageData = async (file) => {
        const image = await createImageBitmap(file);
        const resized = resizeImage(image, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        const epdData = convertImageToEPDData(resized, imageParams);
        return epdData;
    };
    const writeImageData = async (epdData, chunkSize) => {
        await sendDataCommand(CMD_SET_IMAGE_SIZE, [
            (epdData.length >> 24) & 0xff,
            (epdData.length >> 16) & 0xff,
            (epdData.length >> 8) & 0xff,
            epdData.length & 0xff,
        ]);
        let offset = 0;
        while (offset < epdData.length) {
            const size = Math.min(chunkSize, epdData.length - offset);
            const chunk = epdData.slice(offset, offset + size);
            await sendDataCommand(CMD_WRITE_DATA, [
                (offset >> 8) & 0xff,
                offset & 0xff,
                ...chunk,
            ]);
            offset += size;
            const percent = Math.round((offset / epdData.length) * 100);
            setProgress(percent);
            appendStatus(`Đang tải dữ liệu hình ảnh... ${percent}%`);
            await delay(imageParams.confirmInterval);
        }
    };
    const handleConnect = async () => {
        setStatusLines([]);
        setProgress(0);
        if (!bluetoothAvailable) {
            appendStatus("Trinh duyet khong ho tro Web Bluetooth. Hay dung Chrome hoac Edge.");
            return;
        }
        try {
            const bluetooth = navigator
                .bluetooth;
            const device = (await bluetooth.requestDevice({
                filters: [
                    { namePrefix: "IOT-EINK" },
                    { namePrefix: "DLG" },
                    { namePrefix: "EINK" },
                ],
                optionalServices: [SERVICE_UUID],
            }));
            deviceRef.current = device;
            setDeviceName(device.name || "E-Ink Device");
            appendStatus(`Đã tìm thấy thiết bị ${device.name || "không rõ tên"}.`);
            let server = null;
            for (let retries = 3; retries > 0; retries -= 1) {
                try {
                    appendStatus(`Đang kết nối GATT, còn ${retries} lần thử.`);
                    server = (await device.gatt?.connect()) ?? null;
                    if (server) {
                        break;
                    }
                }
                catch (error) {
                    if (retries === 1) {
                        throw error;
                    }
                    await delay(1000);
                }
            }
            if (!server) {
                throw new Error("Không kết nối được GATT server");
            }
            device.addEventListener("gattserverdisconnected", () => {
                appendStatus("Thiết bị đã ngắt kết nối.");
                resetConnection();
            });
            const service = await server.getPrimaryService(SERVICE_UUID);
            controlCharacteristicRef.current =
                await service.getCharacteristic(CHARACTERISTIC_UUID);
            dataCharacteristicRef.current = await service.getCharacteristic(DATA_CHARACTERISTIC_UUID);
            setConnected(true);
            appendStatus("Kết nối thành công.");
        }
        catch (error) {
            appendStatus(`Kết nối thất bại: ${error instanceof Error ? error.message : String(error)}`);
            resetConnection();
        }
    };
    const handleDisconnect = () => {
        const device = deviceRef.current;
        if (device?.gatt?.connected) {
            device.gatt.disconnect();
        }
        appendStatus("Đã ngắt kết nối thiết bị.");
        resetConnection();
    };
    const handleSyncTime = async () => {
        await runBusyTask("Đồng bộ thời gian", "Gửi thời gian hiện tại sang thiết bị", async () => {
            try {
                const now = new Date();
                const unixTime = Math.floor(now.getTime() / 1000) - now.getTimezoneOffset() * 60;
                await sendTimestamp(unixTime);
                await delay(200);
                await sendCommand(0xe2);
                appendStatus(`Đồng bộ thời gian thành công: ${now.toLocaleString("vi-VN")}`);
            }
            catch (error) {
                appendStatus(`Đồng bộ thất bại: ${error instanceof Error ? error.message : String(error)}`);
            }
        });
    };
    const handleStartRotation = () => {
        if (!connected) {
            appendStatus("Cần kết nối thiết bị trước khi bật lịch chuyển màn hình.");
            return;
        }
        if (rotationSelection.length < 2) {
            appendStatus("Cần chọn ít nhất 2 màn hình để tạo lịch xoay.");
            return;
        }
        stopRotation(true);
        rotationIndexRef.current = 0;
        setRotationEnabled(true);
        appendStatus(`Bật lịch xoay màn hình mỗi ${rotationIntervalSec} giây.`);
        rotationTimerRef.current = window.setInterval(() => {
            const nextMode = rotationSelection[rotationIndexRef.current % rotationSelection.length];
            rotationIndexRef.current += 1;
            void sendScreenMode(nextMode).catch((error) => {
                appendStatus(`Lich xoay bi dung: ${error instanceof Error ? error.message : String(error)}`);
                stopRotation(true);
            });
        }, rotationIntervalSec * 1000);
    };
    const toggleRotationSelection = (key) => {
        setRotationSelection((prev) => prev.includes(key) ? prev.filter((item) => item !== key) : [...prev, key]);
    };
    const handleUploadFabric = async () => {
        await runBusyTask("Đang nạp thẻ kho", "Gửi toàn bộ thông tin thẻ kho lên màn hình", async () => {
            try {
                const encoder = new TextEncoder();
                for (let i = 0; i < fabricFields.length; i += 1) {
                    const valueBytes = Array.from(encoder.encode(fabricFields[i].value.trim()));
                    await sendCommand(0xf0, [i, ...valueBytes]);
                    await delay(40);
                }
                await sendScreenMode("fabric");
                appendStatus("Đã nạp thông tin thẻ kho lên thiết bị.");
                setFabricModalOpen(false);
            }
            catch (error) {
                appendStatus(`Nạp thẻ kho thất bại: ${error instanceof Error ? error.message : String(error)}`);
            }
        });
    };
    const handleSendAll = async () => {
        if (!imageFile) {
            appendStatus("Vui lòng chọn một hình ảnh.");
            return;
        }
        await runBusyTask("Loading...", "Xử lý ảnh, ghi dữ liệu, hiển thị và lưu thiết bị", async () => {
            try {
                const epdData = await prepareImageData(imageFile);
                setProgress(0);
                await sendScreenMode("image");
                await delay(300);
                await writeImageData(epdData, 16);
                await sendDataCommand(CMD_DISPLAY_IMAGE);
                await sendDataCommand(CMD_SAVE_IMAGE);
                await delay(800);
                await sendCommand(0xe2);
                appendStatus("Đã hoàn tất toàn bộ quy trình tải ảnh.");
            }
            catch (error) {
                appendStatus(`Tải ảnh thất bại: ${error instanceof Error ? error.message : String(error)}`);
            }
        });
    };
    const onImageFileChange = (event) => {
        setImageFile(event.target.files?.[0] ?? null);
    };
    const onImageFileDrop = (file) => {
        setImageFile(file);
    };
    const updateFabricValue = (id, value) => {
        setFabricFields((prev) => prev.map((field) => (field.id === id ? { ...field, value } : field)));
    };
    const updateImageParam = (key, value) => {
        setImageParams((prev) => ({ ...prev, [key]: value }));
    };
    const resetImageParams = () => {
        setImageParams(initialImageParams);
    };
    const statusText = statusLines.join("\n");
    return (React.createElement("div", { className: "app-shell" },
        React.createElement("div", { className: "background-orb background-orb-a" }),
        React.createElement("div", { className: "background-orb background-orb-b" }),
        React.createElement("div", { className: "app-frame" },
            React.createElement(TopBar, { theme: theme, locale: locale, sidebarOpen: sidebarOpen, onMenuToggle: () => setSidebarOpen((prev) => !prev), onThemeToggle: () => setTheme((prev) => (prev === "light" ? "dark" : "light")), onLocaleToggle: () => setLocale((prev) => (prev === "vi" ? "en" : "vi")) }),
            React.createElement("div", { className: "workspace-shell" },
                React.createElement(ModeRail, { workspaceMode: workspaceMode, locale: locale, open: sidebarOpen, onClose: () => setSidebarOpen(false), onModeChange: setWorkspaceMode }),
                React.createElement("div", { className: "workspace-main" }, workspaceMode === "mode-1" ? (React.createElement("div", { className: "workspace-grid" },
                    React.createElement(ScreenControlPanel, { locale: locale, connected: connected, connecting: connecting, currentTime: currentTime, deviceName: deviceName, bluetoothAvailable: bluetoothAvailable, currentScreenMode: currentScreenMode, rotationEnabled: rotationEnabled, rotationIntervalSec: rotationIntervalSec, rotationSelection: rotationSelection, onConnect: () => void handleConnect(), onDisconnect: handleDisconnect, onSyncTime: () => void handleSyncTime(), onOpenFabricModal: () => setFabricModalOpen(true), onModeSelect: (mode) => void runBusyTask("Đang chuyển màn hình", `Đang chuyển sang ${screenModes.find((item) => item.key === mode)?.label || mode}`, async () => {
                            await sendScreenMode(mode);
                        }), onRotationToggle: toggleRotationSelection, onRotationIntervalChange: setRotationIntervalSec, onRotationStart: handleStartRotation, onRotationStop: () => stopRotation() }),
                    React.createElement(ImageWorkspace, { locale: locale, connected: connected, progress: progress, imageFile: imageFile, imageParams: imageParams, previewRef: previewRef, onFileChange: onImageFileChange, onFileDrop: onImageFileDrop, onImageParamChange: updateImageParam, onResetImageParams: resetImageParams, onSendAll: () => void handleSendAll() }),
                    React.createElement(StatusPanel, { locale: locale, connected: connected, statusText: statusText }))) : workspaceMode === "mode-2" ? (React.createElement(PlaceholderMode, { locale: locale, title: locale === "vi"
                        ? "Mode 2 đang được đặt cho workflow nâng cao"
                        : "Mode 2 reserved for advanced workflows", subtitle: locale === "vi" ? "Mở rộng sau" : "Coming later", points: [
                        locale === "vi"
                            ? "Dashboard xem trước nhiều thiết bị"
                            : "Dashboard for multiple devices",
                        locale === "vi"
                            ? "Đồng bộ dữ liệu theo mẫu template"
                            : "Template-based data sync",
                        locale === "vi"
                            ? "Lịch chạy tự động theo ca làm việc"
                            : "Shift-based scheduling",
                    ] })) : (React.createElement(PlaceholderMode, { locale: locale, title: locale === "vi"
                        ? "Mode 3 đang được đặt cho dashboard bảo trì"
                        : "Mode 3 reserved for maintenance dashboard", subtitle: locale === "vi" ? "Mở rộng sau" : "Coming later", points: [
                        locale === "vi"
                            ? "Chẩn đoán kết nối và log nâng cao"
                            : "Advanced connection diagnostics",
                        locale === "vi"
                            ? "Thư viện giao diện cho từng loại màn hình"
                            : "Screen-specific UI library",
                        locale === "vi"
                            ? "Quản lý preset và profile vận hành"
                            : "Preset and operating profile manager",
                    ] }))))),
        React.createElement(FabricModal, { locale: locale, open: fabricModalOpen, connected: connected, fabricFields: fabricFields, onClose: () => setFabricModalOpen(false), onChange: updateFabricValue, onSubmit: () => void handleUploadFabric() }),
        React.createElement(LoadingOverlay, { loadingState: loadingState })));
}
function ScreenControlPanel(props) {
    const isVi = props.locale === "vi";
    const decreaseRotation = () => {
        props.onRotationIntervalChange(Math.max(5, props.rotationIntervalSec - 5));
    };
    const increaseRotation = () => {
        props.onRotationIntervalChange(props.rotationIntervalSec + 5);
    };
    const getModeIcon = (mode) => {
        if (mode === "calendar") {
            return (React.createElement("svg", { viewBox: "0 0 24 24", "aria-hidden": "true" },
                React.createElement("rect", { x: "4", y: "5", width: "16", height: "15", rx: "3" }),
                React.createElement("path", { d: "M8 3v4M16 3v4M4 10h16" })));
        }
        if (mode === "clock") {
            return (React.createElement("svg", { viewBox: "0 0 24 24", "aria-hidden": "true" },
                React.createElement("circle", { cx: "12", cy: "12", r: "8" }),
                React.createElement("path", { d: "M12 8v5l3 2" })));
        }
        if (mode === "calendarAnalog") {
            return (React.createElement("svg", { viewBox: "0 0 24 24", "aria-hidden": "true" },
                React.createElement("rect", { x: "3", y: "4", width: "12", height: "14", rx: "3" }),
                React.createElement("path", { d: "M6 2v4M12 2v4M3 9h12" }),
                React.createElement("circle", { cx: "18", cy: "16", r: "3.5" }),
                React.createElement("path", { d: "M18 14.6v1.7l1.1.8" })));
        }
        if (mode === "fabric") {
            return (React.createElement("svg", { viewBox: "0 0 24 24", "aria-hidden": "true" },
                React.createElement("rect", { x: "4", y: "4", width: "10", height: "16", rx: "2" }),
                React.createElement("path", { d: "M8 8h2M8 12h2M8 16h2M16 7v10M19 7v10" })));
        }
        return (React.createElement("svg", { viewBox: "0 0 24 24", "aria-hidden": "true" },
            React.createElement("rect", { x: "4", y: "5", width: "16", height: "14", rx: "2" }),
            React.createElement("path", { d: "M7 9h10M7 12h10M7 15h6" })));
    };
    const getModeLabel = (mode) => {
        const labels = {
            image: isVi ? "Hình ảnh" : "Image",
            calendar: isVi ? "Lịch" : "Calendar",
            clock: isVi ? "Đồng hồ" : "Clock",
            calendarAnalog: isVi ? "Lịch & đồng hồ" : "Calendar & clock",
            fabric: isVi ? "Thẻ kho" : "Fabric card",
        };
        return labels[mode];
    };
    const getModeDescription = (mode) => {
        const descriptions = {
            image: isVi ? "Nội dung ảnh E-Ink" : "E-Ink image content",
            calendar: isVi ? "Giao diện lịch cơ bản" : "Basic calendar layout",
            clock: isVi ? "Hiển thị thời gian" : "Time display",
            calendarAnalog: isVi ? "Lịch kết hợp đồng hồ" : "Calendar and clock",
            fabric: isVi ? "Biểu mẫu thẻ kho" : "Fabric card form",
        };
        return descriptions[mode];
    };
    const connectionTitle = props.connected
        ? isVi
            ? "Thiết bị đang sẵn sàng"
            : "Device ready"
        : isVi
            ? "Thiết bị chưa kết nối"
            : "Device offline";
    return (React.createElement("section", { className: "panel-card operations-suite panel-span-two" },
        React.createElement("div", { className: "operations-suite-head" },
            React.createElement("div", null,
                React.createElement("span", { className: "eyebrow" }, isVi ? "Điều khiển trung tâm" : "Central control"),
                React.createElement("h2", null, isVi
                    ? "Điều khiển thiết bị và chế độ màn hình"
                    : "Device and display control")),
            React.createElement("div", { className: props.connected ? "status-tag is-online" : "status-tag is-offline" }, props.connected ? (isVi ? "Đang hoạt động" : "Online") : "Offline")),
        React.createElement("div", { className: "operations-suite-grid" },
            React.createElement("div", { className: "ble-console" },
                React.createElement("div", { className: "ble-console-top" },
                    React.createElement("div", { className: "ble-console-indicator" },
                        React.createElement("span", { className: props.connected ? "ble-dot is-online" : "ble-dot is-offline" })),
                    React.createElement("div", { className: "ble-console-copy" },
                        React.createElement("span", { className: "ble-console-kicker" }, "Bluetooth LE"),
                        React.createElement("h3", null, connectionTitle))),
                React.createElement("div", { className: "ble-console-stats" },
                    React.createElement("div", { className: "ble-stat" },
                        React.createElement("span", null, isVi ? "Thiết bị" : "Device"),
                        React.createElement("strong", null, props.deviceName)),
                    React.createElement("div", { className: "ble-stat" },
                        React.createElement("span", null, isVi ? "Thời gian hiện tại" : "Local time"),
                        React.createElement("strong", null, props.currentTime)),
                    React.createElement("div", { className: "ble-stat" },
                        React.createElement("span", null, isVi ? "Môi trường" : "Environment"),
                        React.createElement("strong", null, props.bluetoothAvailable
                            ? "Web Bluetooth"
                            : isVi
                                ? "Không hỗ trợ"
                                : "Unsupported"))),
                React.createElement("div", { className: "ble-console-actions" },
                    React.createElement("button", { className: "primary-button", disabled: props.connected || props.connecting, onClick: props.onConnect }, props.connected
                        ? isVi
                            ? "Đã kết nối"
                            : "Connected"
                        : props.connecting
                            ? isVi
                                ? "Đang kết nối..."
                                : "Connecting..."
                            : isVi
                                ? "Kết nối"
                                : "Connect"),
                    React.createElement("button", { className: "ghost-button", disabled: !props.connected, onClick: props.onDisconnect }, isVi ? "Ngắt kết nối" : "Disconnect"),
                    React.createElement("button", { className: "ghost-button", disabled: !props.connected, onClick: props.onSyncTime }, isVi ? "Đồng bộ thời gian" : "Sync time"))),
            React.createElement("div", { className: "display-console" },
                React.createElement("div", { className: "display-console-head" },
                    React.createElement("div", null,
                        React.createElement("span", { className: "eyebrow" }, isVi ? "Chế độ hiển thị" : "Display modes"),
                        React.createElement("h3", null, isVi ? "Chọn giao diện màn hình" : "Choose a display mode")),
                    React.createElement("div", { className: "current-mode-pill" }, getModeLabel(props.currentScreenMode))),
                React.createElement("div", { className: "display-mode-grid" }, screenModes.map((mode) => (React.createElement("button", { key: mode.key, className: props.currentScreenMode === mode.key
                        ? "display-mode-card is-active"
                        : "display-mode-card", disabled: !props.connected, onClick: () => mode.key === "fabric"
                        ? props.onOpenFabricModal()
                        : props.onModeSelect(mode.key) },
                    React.createElement("span", { className: "display-mode-icon" }, getModeIcon(mode.key)),
                    React.createElement("strong", null, getModeLabel(mode.key)),
                    React.createElement("small", null, getModeDescription(mode.key)))))))),
        React.createElement("div", { className: "rotation-console" },
            React.createElement("div", { className: "rotation-console-head" },
                React.createElement("div", null,
                    React.createElement("span", { className: "eyebrow" }, isVi ? "Luân chuyển tự động" : "Auto rotation"),
                    React.createElement("h3", null, isVi
                        ? "Thiết lập chu kỳ đổi màn hình"
                        : "Configure scheduled screen rotation")),
                React.createElement("div", { className: props.rotationEnabled
                        ? "rotation-state is-running"
                        : "rotation-state" },
                    React.createElement("span", { className: "rotation-state-icon", "aria-hidden": "true" }, props.rotationEnabled ? "▶" : "||"),
                    React.createElement("span", null, props.rotationEnabled
                        ? isVi
                            ? "Đang luân chuyển"
                            : "Rotating"
                        : isVi
                            ? "Tạm dừng"
                            : "Paused"))),
            React.createElement("div", { className: "rotation-chip-grid" }, screenModes.map((mode) => (React.createElement("label", { className: "rotation-chip", key: mode.key },
                React.createElement("input", { type: "checkbox", checked: props.rotationSelection.includes(mode.key), onChange: () => props.onRotationToggle(mode.key) }),
                React.createElement("span", null, getModeLabel(mode.key)))))),
            React.createElement("div", { className: "rotation-controls-pro" },
                React.createElement("div", { className: "setting-group compact rotation-interval-group" },
                    React.createElement("label", { htmlFor: "rotationInterval" }, isVi ? "Chu kỳ xoay (giây)" : "Rotation interval (seconds)"),
                    React.createElement("div", { className: "stepper-input" },
                        React.createElement("button", { type: "button", className: "icon-button stepper-button", onClick: decreaseRotation }, "-"),
                        React.createElement("input", { id: "rotationInterval", type: "number", min: "5", step: "5", value: props.rotationIntervalSec, onChange: (event) => props.onRotationIntervalChange(Number(event.target.value) || 5) }),
                        React.createElement("button", { type: "button", className: "icon-button stepper-button", onClick: increaseRotation }, "+"))),
                React.createElement("div", { className: "rotation-action-row" },
                    React.createElement("button", { className: "primary-button", disabled: !props.connected, onClick: props.onRotationStart }, isVi ? "Bắt đầu luân chuyển" : "Start rotation"),
                    React.createElement("button", { className: "ghost-button", disabled: !props.rotationEnabled, onClick: props.onRotationStop }, isVi ? "Dừng luân chuyển" : "Stop rotation"))))));
}
function FabricModal(props) {
    if (!props.open) {
        return null;
    }
    const isVi = props.locale === "vi";
    return (React.createElement("div", { className: "modal-backdrop", onClick: props.onClose },
        React.createElement("div", { className: "modal-card", onClick: (event) => event.stopPropagation() },
            React.createElement("div", { className: "panel-head" },
                React.createElement("div", null,
                    React.createElement("span", { className: "eyebrow" }, isVi ? "Thẻ kho" : "Fabric card"),
                    React.createElement("h2", null, isVi ? "Nhập dữ liệu thẻ kho" : "Enter fabric card data")),
                React.createElement("button", { className: "icon-button", onClick: props.onClose }, isVi ? "Đóng" : "Close")),
            React.createElement("div", { className: "fabric-form fabric-form-modal" }, props.fabricFields.map((field) => (React.createElement("div", { className: "setting-group", key: field.id },
                React.createElement("label", { htmlFor: field.id }, field.label),
                React.createElement("input", { id: field.id, type: "text", value: field.value, onChange: (event) => props.onChange(field.id, event.target.value) }))))),
            React.createElement("div", { className: "modal-footer-actions" },
                React.createElement("button", { className: "ghost-button", onClick: props.onClose }, isVi ? "Hủy" : "Cancel"),
                React.createElement("button", { className: "primary-button", disabled: !props.connected, onClick: props.onSubmit }, isVi ? "Xác nhận và nạp lên màn hình" : "Confirm and upload")))));
}
function ImageWorkspace(props) {
    const isVi = props.locale === "vi";
    const [isDragOver, setIsDragOver] = React.useState(false);
    const ditheringHint = (() => {
        if (props.imageParams.dithering === "ordered") {
            return isVi
                ? "Ordered: tạo mẫu hạt đều, xử lý nhanh và ổn định cho ảnh đồ họa."
                : "Ordered: creates an even dot pattern, fast and stable for graphic-style images.";
        }
        if (props.imageParams.dithering === "floyd-steinberg") {
            return isVi
                ? "Floyd-Steinberg: phân tán sai số để ảnh mềm và tự nhiên hơn, hợp với ảnh chụp."
                : "Floyd-Steinberg: diffuses error for softer, more natural-looking results, good for photos.";
        }
        return isVi
            ? "Không dùng: giữ ngưỡng đen trắng cơ bản, phù hợp khi muốn ảnh sắc và ít nhiễu."
            : "None: keeps a basic black-and-white threshold, useful when you want a sharper and cleaner result.";
    })();
    const handleDrop = (event) => {
        event.preventDefault();
        setIsDragOver(false);
        const file = event.dataTransfer.files?.[0];
        if (file) {
            props.onFileDrop(file);
        }
    };
    return (React.createElement("section", { className: "panel-card image-panel-pro panel-span-two" },
        React.createElement("div", { className: "panel-head" },
            React.createElement("div", null,
                React.createElement("span", { className: "eyebrow" }, isVi ? "Hình ảnh" : "Image studio"),
                React.createElement("h2", null, isVi ? "Xử lý và nạp nội dung E-Ink" : "Process and deploy E-Ink content"))),
        React.createElement("div", { className: "image-studio-grid" },
            React.createElement("div", { className: "image-studio-left" },
                React.createElement("label", { className: isDragOver ? "dropzone-pro is-drag-over" : "dropzone-pro", onDragOver: (event) => {
                        event.preventDefault();
                        setIsDragOver(true);
                    }, onDragLeave: () => setIsDragOver(false), onDrop: handleDrop },
                    React.createElement("span", { className: "dropzone-icon", "aria-hidden": "true" },
                        React.createElement("svg", { viewBox: "0 0 24 24" },
                            React.createElement("path", { d: "M12 16V8" }),
                            React.createElement("path", { d: "M8.8 11.2 12 8l3.2 3.2" }),
                            React.createElement("path", { d: "M6 17.5h12" }),
                            React.createElement("path", { d: "M7.5 19.5h9" }))),
                    React.createElement("span", { className: "eyebrow" }, isVi ? "Kéo thả ảnh" : "Drag and drop"),
                    React.createElement("strong", null, isVi ? "Thả hình ảnh vào đây hoặc bấm để chọn file" : "Drop an image here or click to choose a file"),
                    React.createElement("small", null, props.imageFile
                        ? props.imageFile.name
                        : isVi
                            ? "Hỗ trợ ảnh dùng để chuyển sang màn hình E-Ink"
                            : "Supports images that will be converted for the E-Ink display"),
                    React.createElement("input", { type: "file", accept: "image/*", onChange: props.onFileChange })),
                React.createElement("div", { className: "image-controls-head" },
                    React.createElement("div", null,
                        React.createElement("h3", null, isVi ? "Tinh chỉnh hình ảnh" : "Image adjustments"),
                        React.createElement("p", null, isVi ? "Điều chỉnh nhanh trước khi tải lên thiết bị." : "Quick adjustments before uploading to the device.")),
                    React.createElement("button", { type: "button", className: "icon-button reset-button", onClick: props.onResetImageParams }, "\u21BA")),
                React.createElement("div", { className: "image-tuning-card image-tuning-plain" },
                    React.createElement("div", { className: "setting-group" },
                        React.createElement("label", { htmlFor: "brightness" },
                            isVi ? "Độ sáng" : "Brightness",
                            ": ",
                            props.imageParams.brightness,
                            "%"),
                        React.createElement("input", { id: "brightness", type: "range", min: "0", max: "200", value: props.imageParams.brightness, onChange: (event) => props.onImageParamChange("brightness", Number(event.target.value)) })),
                    React.createElement("div", { className: "setting-group" },
                        React.createElement("label", { htmlFor: "contrast" },
                            isVi ? "Tương phản" : "Contrast",
                            ": ",
                            props.imageParams.contrast,
                            "%"),
                        React.createElement("input", { id: "contrast", type: "range", min: "0", max: "200", value: props.imageParams.contrast, onChange: (event) => props.onImageParamChange("contrast", Number(event.target.value)) })),
                    React.createElement("div", { className: "setting-group compact" },
                        React.createElement("label", { htmlFor: "dithering" }, isVi ? "Kiểu dithering" : "Dithering type"),
                        React.createElement("select", { id: "dithering", value: props.imageParams.dithering, onChange: (event) => props.onImageParamChange("dithering", event.target.value) },
                            React.createElement("option", { value: "none" }, isVi ? "Không dùng" : "None"),
                            React.createElement("option", { value: "ordered" }, "Ordered"),
                            React.createElement("option", { value: "floyd-steinberg" }, "Floyd-Steinberg")),
                        React.createElement("small", { className: "setting-hint" }, ditheringHint)),
                    React.createElement("div", { className: "setting-group" },
                        React.createElement("label", { htmlFor: "ditherIntensity" },
                            isVi ? "Cường độ dithering" : "Dithering strength",
                            ": ",
                            props.imageParams.ditherIntensity.toFixed(1)),
                        React.createElement("input", { id: "ditherIntensity", type: "range", min: "0", max: "2", step: "0.1", value: props.imageParams.ditherIntensity, onChange: (event) => props.onImageParamChange("ditherIntensity", Number(event.target.value)) }))),
                React.createElement("button", { className: "primary-button upload-hero-button", disabled: !props.connected || !props.imageFile, onClick: props.onSendAll }, isVi ? "Xử lý và tải ảnh lên thiết bị" : "Process and upload to device")),
            React.createElement("div", { className: "image-studio-right" },
                React.createElement("div", { className: "preview-stage preview-stage-large" },
                    React.createElement("div", { className: "preview-head" },
                        React.createElement("div", null,
                            React.createElement("h3", null, isVi ? "Xem trước 212x104" : "212x104 preview"),
                            React.createElement("span", null, isVi ? "Ảnh hiển thị theo chuẩn đơn sắc E-Ink" : "Monochrome E-Ink rendering"))),
                    React.createElement("canvas", { ref: props.previewRef, width: DISPLAY_WIDTH, height: DISPLAY_HEIGHT, className: "preview-canvas" })))),
        React.createElement("div", { className: "progress-shell" },
            React.createElement("div", { className: "progress-labels" },
                React.createElement("span", null, isVi ? "Tiến trình tải lên" : "Upload progress"),
                React.createElement("strong", null,
                    props.progress,
                    "%")),
            React.createElement("div", { id: "progress" },
                React.createElement("div", { id: "progress-bar", style: { width: `${props.progress}%` } })))));
}
/**
 * Nút chuyển đổi giao diện Sáng/Tối
 */
const ThemeToggle = ({ theme, toggleTheme, }) => (React.createElement("div", { style: {
        display: "flex",
        alignItems: "center",
        justifyContent: "flex-end",
        gap: "0.4rem",
        marginBottom: "0.2rem",
        marginRight: 0,
    } },
    React.createElement("span", { className: "theme-icon-emoji", style: { fontSize: "1rem" } }, theme === "dark" ? "🌙" : "☀️"),
    React.createElement("label", { className: "theme-switch", htmlFor: "checkbox" },
        React.createElement("input", { type: "checkbox", id: "checkbox", onChange: toggleTheme, checked: theme === "light" }),
        React.createElement("div", { className: "slider round" }))));
const FlagIcon = ({ locale }) => {
    if (locale === "vi") {
        return (React.createElement("svg", { viewBox: "0 0 24 16", "aria-hidden": "true" },
            React.createElement("rect", { width: "24", height: "16", rx: "2", fill: "#da251d" }),
            React.createElement("path", { d: "M12 3.2 13.4 7.2h4.2l-3.4 2.4 1.3 4-3.5-2.5-3.5 2.5 1.3-4-3.4-2.4h4.2Z", fill: "#ffde00" })));
    }
    return (React.createElement("svg", { viewBox: "0 0 24 16", "aria-hidden": "true" },
        React.createElement("rect", { width: "24", height: "16", rx: "2", fill: "#012169" }),
        React.createElement("path", { d: "M0 0 24 16M24 0 0 16", stroke: "#fff", strokeWidth: "4" }),
        React.createElement("path", { d: "M0 0 24 16M24 0 0 16", stroke: "#C8102E", strokeWidth: "2" }),
        React.createElement("path", { d: "M12 0v16M0 8h24", stroke: "#fff", strokeWidth: "6" }),
        React.createElement("path", { d: "M12 0v16M0 8h24", stroke: "#C8102E", strokeWidth: "3.2" })));
};
function TopBar(props) {
    const { theme, locale, sidebarOpen, onMenuToggle, onThemeToggle, onLocaleToggle, } = props;
    const isVi = locale === "vi";
    return (React.createElement("header", { className: "topbar" },
        React.createElement("div", { className: "topbar-left" },
            React.createElement("button", { type: "button", className: sidebarOpen ? "menu-button is-active" : "menu-button", onClick: onMenuToggle, "aria-label": isVi ? "Mở thanh điều hướng" : "Open navigation" },
                React.createElement("span", null),
                React.createElement("span", null),
                React.createElement("span", null)),
            React.createElement("div", { className: "topbar-brandline" },
                React.createElement("div", { className: "topbar-copy" },
                    React.createElement("strong", null, "DA14585 E-Ink"),
                    React.createElement("span", null, "Device dashboard")))),
        React.createElement("div", { className: "topbar-title" }, isVi
            ? "Bảng điều khiển thiết bị E-Ink"
            : "E-Ink device control dashboard"),
        React.createElement("div", { className: "topbar-right" },
            React.createElement("button", { className: "language-button", onClick: onLocaleToggle },
                React.createElement("span", { className: "flag-icon" },
                    React.createElement(FlagIcon, { locale: isVi ? "vi" : "en" })),
                React.createElement("strong", null, isVi ? "vn" : "en")),
            React.createElement(ThemeToggle, { theme: theme, toggleTheme: onThemeToggle }))));
}
function ModeRail(props) {
    const isVi = props.locale === "vi";
    const getNavIcon = (key) => {
        if (key === "mode-1") {
            return (React.createElement("svg", { viewBox: "0 0 24 24", "aria-hidden": "true" },
                React.createElement("path", { d: "M4 7h10" }),
                React.createElement("path", { d: "M4 12h16" }),
                React.createElement("path", { d: "M4 17h8" }),
                React.createElement("circle", { cx: "17", cy: "7", r: "2" }),
                React.createElement("circle", { cx: "9", cy: "17", r: "2" })));
        }
        if (key === "mode-2") {
            return (React.createElement("svg", { viewBox: "0 0 24 24", "aria-hidden": "true" },
                React.createElement("rect", { x: "4", y: "5", width: "10", height: "10", rx: "2" }),
                React.createElement("rect", { x: "10", y: "9", width: "10", height: "10", rx: "2" })));
        }
        return (React.createElement("svg", { viewBox: "0 0 24 24", "aria-hidden": "true" },
            React.createElement("path", { d: "M12 3v4" }),
            React.createElement("path", { d: "M12 17v4" }),
            React.createElement("path", { d: "M4.9 4.9l2.8 2.8" }),
            React.createElement("path", { d: "M16.3 16.3l2.8 2.8" }),
            React.createElement("path", { d: "M3 12h4" }),
            React.createElement("path", { d: "M17 12h4" }),
            React.createElement("path", { d: "M4.9 19.1l2.8-2.8" }),
            React.createElement("path", { d: "M16.3 7.7l2.8-2.8" }),
            React.createElement("circle", { cx: "12", cy: "12", r: "3.5" })));
    };
    const items = [
        {
            key: "mode-1",
            title: isVi ? "Điều khiển thiết bị" : "Device control",
            desc: isVi ? "BLE, màn hình, ảnh" : "BLE, display, image",
        },
        {
            key: "mode-2",
            title: isVi ? "Quy trình nâng cao" : "Advanced workflows",
            desc: isVi ? "Mở rộng sau" : "Coming later",
        },
        {
            key: "mode-3",
            title: isVi ? "Bảo trì & preset" : "Maintenance & presets",
            desc: isVi ? "Chẩn đoán" : "Diagnostics",
        },
    ];
    return (React.createElement(React.Fragment, null,
        React.createElement("div", { className: props.open ? "sidebar-backdrop is-open" : "sidebar-backdrop", onClick: props.onClose }),
        React.createElement("aside", { className: props.open ? "sidebar-panel is-open" : "sidebar-panel" },
            React.createElement("nav", { className: "sidebar-nav" }, items.map((item) => (React.createElement("button", { key: item.key, className: props.workspaceMode === item.key
                    ? "sidebar-nav-item is-active"
                    : "sidebar-nav-item", onClick: () => {
                    props.onModeChange(item.key);
                    props.onClose();
                } },
                React.createElement("span", { className: "sidebar-nav-index" }, getNavIcon(item.key)),
                React.createElement("span", { className: "sidebar-nav-copy" },
                    React.createElement("strong", null, item.title),
                    React.createElement("small", null, item.desc)))))))));
}
function HeroSection(props) {
    const isVi = props.locale === "vi";
    return (React.createElement("section", { className: "panel-card panel-span-two" },
        React.createElement("div", { className: "panel-head" },
            React.createElement("div", null,
                React.createElement("span", { className: "eyebrow" }, isVi ? "Tổng quan" : "Overview"),
                React.createElement("h2", null, props.connected
                    ? isVi
                        ? "Thiết bị đang trực tuyến"
                        : "Device online"
                    : isVi
                        ? "Thiết bị chưa kết nối"
                        : "Device offline")))));
}
function StatusPanel(props) {
    const isVi = props.locale === "vi";
    return (React.createElement("section", { className: "panel-card status-panel" },
        React.createElement("div", { className: "panel-head" },
            React.createElement("div", null,
                React.createElement("span", { className: "eyebrow" }, isVi ? "Luồng nhật ký" : "Log stream"),
                React.createElement("h2", null, isVi ? "Nhật ký thiết bị" : "Device log")),
            React.createElement("div", { className: props.connected ? "status-tag is-online" : "status-tag is-offline" }, props.connected ? (isVi ? "Đang hoạt động" : "Active") : "Offline")),
        React.createElement("div", { id: "status" }, props.statusText)));
}
function PlaceholderMode(props) {
    const isVi = props.locale === "vi";
    return (React.createElement("div", { className: "workspace-grid placeholder-grid" },
        React.createElement("section", { className: "panel-card placeholder-hero" },
            React.createElement("span", { className: "eyebrow" }, props.subtitle),
            React.createElement("h1", null, props.title),
            React.createElement("p", null, isVi
                ? "Mode này đang được giữ chỗ để hoàn thiện thành một module riêng với luồng thao tác độc lập."
                : "This mode is reserved for a dedicated module with its own workflow.")),
        props.points.map((point, index) => (React.createElement("section", { className: "panel-card", key: point },
            React.createElement("span", { className: "eyebrow" }, isVi ? `Ý tưởng ${index + 1}` : `Idea ${index + 1}`),
            React.createElement("h2", null, point),
            React.createElement("p", null, isVi
                ? "Khi chốt chức năng thật, phần này có thể được mở rộng mà không cần thay lại khung tổng."
                : "Once the feature set is defined, this section can expand without redesigning the shell."))))));
}
function LoadingOverlay(props) {
    if (!props.loadingState.active) {
        return null;
    }
    return (React.createElement("div", { className: "loading-overlay" },
        React.createElement("div", { className: "loading-card" },
            React.createElement("div", { className: "spinner-ring" }),
            React.createElement("h3", null, props.loadingState.title),
            React.createElement("p", null, props.loadingState.detail))));
}
ReactDOM.render(React.createElement(React.StrictMode, null,
    React.createElement(App, null)), document.getElementById("root"));
//# sourceMappingURL=app.js.map