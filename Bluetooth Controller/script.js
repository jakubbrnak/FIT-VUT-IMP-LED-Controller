let bluetoothDevice;
let animationCharacteristic;
let speedCharacteristic;

document.getElementById('connect').addEventListener('click', () => {
    navigator.bluetooth.requestDevice({
        acceptAllDevices: true,
        optionalServices: ['aa7113df-59cc-4456-adb1-ccd5d5c5f5ef']
    })
    .then(device => {
        bluetoothDevice = device;
        return device.gatt.connect();
    })
    .then(server => {
        return Promise.all([
            server.getPrimaryService('aa7113df-59cc-4456-adb1-ccd5d5c5f5ef').then(service => {
                return service.getCharacteristic('2f845da3-ce4a-41d5-822a-02513d2ddcf4');
            })
        ]);
    })
    .then(characteristics => {
        [animationCharacteristic, speedCharacteristic] = characteristics;
        document.getElementById('rgb-controls').style.display = 'block';
        document.getElementById('rgb-speed').style.display = 'block';
        document.getElementById('simple-controls').style.display = 'block';
        document.getElementById('simple-speed').style.display = 'block';
    })
    .catch(error => {
        console.error('Bluetooth connection failed:', error);
    });
});

function sendValue(value) {
    if (animationCharacteristic) {
        animationCharacteristic.writeValue(new TextEncoder().encode(String(value)));
    }
}

