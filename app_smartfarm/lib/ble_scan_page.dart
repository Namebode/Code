import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';

class BLEScanPage extends StatefulWidget {
  const BLEScanPage({Key? key}) : super(key: key);

  @override
  State<BLEScanPage> createState() => _BLEScanPageState();
}

class _BLEScanPageState extends State<BLEScanPage> {
  bool isScanning = false;
  List<ScanResult> devices = [];
  BluetoothDevice? connectedDevice;
  TextEditingController deviceIdController = TextEditingController();
  BluetoothCharacteristic? writeCharacteristic;

  @override
  void initState() {
    super.initState();
    checkPermissions();
  }

  Future<void> checkPermissions() async {
    if (await Permission.bluetooth.isDenied ||
        await Permission.location.isDenied ||
        await Permission.bluetoothScan.isDenied ||
        await Permission.bluetoothConnect.isDenied) {
      await [
        Permission.bluetooth,
        Permission.location,
        Permission.bluetoothScan,
        Permission.bluetoothConnect,
      ].request();
    }
  }

  Future<void> startScan() async {
    await checkPermissions();
    setState(() {
      isScanning = true;
      devices.clear();
    });

    try {
      await FlutterBluePlus.startScan(timeout: const Duration(seconds: 5));
      FlutterBluePlus.scanResults.listen((results) {
        if (mounted) {
          setState(() {
            devices = results;
          });
        }
      });
    } catch (e) {
      print('Error during scan: $e');
    } finally {
      setState(() {
        isScanning = false;
      });
    }
  }

  Future<void> connectToDevice(BluetoothDevice device) async {
    setState(() {
      connectedDevice = device;
    });

    try {
      await device.connect();
      List<BluetoothService> services = await device.discoverServices();
      for (var service in services) {
        for (var characteristic in service.characteristics) {
          if (characteristic.properties.write) {
            setState(() {
              writeCharacteristic = characteristic;
            });
            break;
          }
        }
      }
    } catch (e) {
      print('Connection error: $e');
    }
  }

  Future<void> showConnectDialog(BluetoothDevice device) async {
    // แสดง Dialog เพื่อยืนยันการเชื่อมต่อ
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text("Confirm Connection"),
        content: Text(
            "Do you want to connect to this device?\nDevice ID: ${device.id}"),
        actions: [
          TextButton(
            onPressed: () {
              Navigator.pop(context);
            },
            child: const Text("Cancel"),
          ),
          TextButton(
            onPressed: () {
              Navigator.pop(context);
              connectToDevice(device);
            },
            child: const Text("Connect"),
          ),
        ],
      ),
    );
  }

  Future<void> sendDeviceId(String deviceId) async {
    if (writeCharacteristic != null) {
      String command = "SET_ID:$deviceId\n";
      await writeCharacteristic!.write(command.codeUnits);
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text("Device ID updated successfully!")),
      );
    }
  }

  Future<void> disconnectDevice() async {
    if (connectedDevice != null) {
      await connectedDevice!.disconnect();
      setState(() {
        connectedDevice = null;
      });
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text("Disconnected from the device")),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text("BLE Device Config"),
        elevation: 0,
        backgroundColor: const Color.fromARGB(255, 140, 90, 197),
      ),
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            colors: [
              Color.fromARGB(255, 140, 90, 197),
              Color.fromARGB(255, 99, 46, 247)
            ],
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
          ),
        ),
        child: SafeArea(
          child: Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            child: Column(
              children: [
                const SizedBox(height: 16),
                // Status Card with Connected Device Info
                Card(
                  elevation: 8,
                  shadowColor: Colors.black26,
                  shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(15),
                  ),
                  child: Padding(
                    padding: const EdgeInsets.all(16.0),
                    child: Column(
                      children: [
                        Row(
                          children: [
                            Container(
                              padding: const EdgeInsets.all(12),
                              decoration: BoxDecoration(
                                color: connectedDevice != null
                                    ? Colors.green.shade50
                                    : (isScanning
                                        ? Colors.blue.shade50
                                        : Colors.indigo.shade50),
                                borderRadius: BorderRadius.circular(12),
                              ),
                              child: Icon(
                                connectedDevice != null
                                    ? Icons.bluetooth_connected
                                    : (isScanning
                                        ? Icons.bluetooth_searching
                                        : Icons.bluetooth),
                                color: connectedDevice != null
                                    ? Colors.green
                                    : (isScanning
                                        ? Colors.blue
                                        : Colors.indigo),
                                size: 28,
                              ),
                            ),
                            const SizedBox(width: 16),
                            Expanded(
                              child: Column(
                                crossAxisAlignment: CrossAxisAlignment.start,
                                children: [
                                  Text(
                                    connectedDevice != null
                                        ? "เชื่อมต่อกับ: ${connectedDevice!.name}"
                                        : (isScanning
                                            ? "กำลังค้นหา..."
                                            : "ผลการค้นหา"),
                                    style: const TextStyle(
                                      fontSize: 18,
                                      fontWeight: FontWeight.bold,
                                    ),
                                  ),
                                  Text(
                                    connectedDevice != null
                                        ? "ID: ${connectedDevice!.id}"
                                        : "พบ ${devices.length} อุปกรณ์",
                                    style: TextStyle(
                                      color: Colors.grey[600],
                                      fontSize: 14,
                                    ),
                                  ),
                                ],
                              ),
                            ),
                            if (connectedDevice != null)
                              IconButton(
                                onPressed: disconnectDevice,
                                icon: const Icon(Icons.close),
                                color: Colors.red,
                              ),
                          ],
                        ),
                        // Device ID Configuration
                        if (connectedDevice != null) ...[
                          const SizedBox(height: 16),
                          const Divider(),
                          const SizedBox(height: 16),
                          TextField(
                            controller: deviceIdController,
                            decoration: InputDecoration(
                              labelText: "ป้อน Device ID",
                              border: OutlineInputBorder(
                                borderRadius: BorderRadius.circular(12),
                              ),
                              filled: true,
                              fillColor: Colors.grey.shade50,
                              prefixIcon: const Icon(Icons.edit),
                              suffixIcon: IconButton(
                                icon: const Icon(Icons.send),
                                onPressed: () => sendDeviceId(
                                    deviceIdController.text.trim()),
                              ),
                            ),
                          ),
                        ],
                      ],
                    ),
                  ),
                ),
                const SizedBox(height: 16),

                // Devices List (ส่วนที่เหลือเหมือนเดิม)
                if (isScanning)
                  const Expanded(
                    child: Center(
                      child: Column(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          CircularProgressIndicator(color: Colors.white),
                          SizedBox(height: 16),
                          Text(
                            "กำลังค้นหาอุปกรณ์...",
                            style: TextStyle(color: Colors.white, fontSize: 16),
                          ),
                        ],
                      ),
                    ),
                  )
                else if (devices.isEmpty)
                  Expanded(
                    child: Card(
                      elevation: 8,
                      shadowColor: Colors.black26,
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(15),
                      ),
                      child: Center(
                        child: Column(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            Icon(
                              Icons.bluetooth_disabled,
                              size: 64,
                              color: Colors.grey[400],
                            ),
                            const SizedBox(height: 16),
                            Text(
                              "ไม่พบอุปกรณ์",
                              style: TextStyle(
                                fontSize: 18,
                                color: Colors.grey[600],
                                fontWeight: FontWeight.bold,
                              ),
                            ),
                            const SizedBox(height: 8),
                            const Text(
                              "กดปุ่มค้นหาเพื่อเริ่มต้นใหม่",
                              style: TextStyle(color: Colors.grey),
                            ),
                          ],
                        ),
                      ),
                    ),
                  )
                else
                  Expanded(
                    child: Card(
                      elevation: 8,
                      shadowColor: Colors.black26,
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(15),
                      ),
                      child: ClipRRect(
                        borderRadius: BorderRadius.circular(15),
                        child: DefaultTabController(
                          length: 2,
                          child: Column(
                            children: [
                              Container(
                                color: Colors.indigo.shade50,
                                child: TabBar(
                                  tabs: [
                                    Tab(
                                      child: Row(
                                        mainAxisSize: MainAxisSize.min,
                                        children: [
                                          const Icon(Icons.bluetooth),
                                          const SizedBox(width: 8),
                                          Text(
                                            "ระบุชื่อ (${devices.where((result) => result.advertisementData.localName.isNotEmpty).length})",
                                          ),
                                        ],
                                      ),
                                    ),
                                    Tab(
                                      child: Row(
                                        mainAxisSize: MainAxisSize.min,
                                        children: [
                                          const Icon(Icons.bluetooth_disabled),
                                          const SizedBox(width: 8),
                                          Text(
                                            "ไม่ระบุ (${devices.where((result) => result.advertisementData.localName.isEmpty).length})",
                                          ),
                                        ],
                                      ),
                                    ),
                                  ],
                                  labelColor: Colors.indigo,
                                  unselectedLabelColor: Colors.grey,
                                  indicatorColor: Colors.indigo,
                                ),
                              ),
                              Expanded(
                                child: TabBarView(
                                  children: [
                                    _buildDevicesList(
                                      devices
                                          .where((result) => result
                                              .advertisementData
                                              .localName
                                              .isNotEmpty)
                                          .toList(),
                                      true,
                                    ),
                                    _buildDevicesList(
                                      devices
                                          .where((result) => result
                                              .advertisementData
                                              .localName
                                              .isEmpty)
                                          .toList(),
                                      false,
                                    ),
                                  ],
                                ),
                              ),
                            ],
                          ),
                        ),
                      ),
                    ),
                  ),

                const SizedBox(height: 16),
                // Scan Button
                SizedBox(
                  width: double.infinity,
                  height: 56,
                  child: ElevatedButton.icon(
                    onPressed: isScanning ? null : startScan,
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.white,
                      foregroundColor: Colors.indigo,
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(15),
                      ),
                      elevation: 8,
                      shadowColor: Colors.black26,
                    ),
                    icon:
                        Icon(isScanning ? Icons.hourglass_empty : Icons.search),
                    label: Text(
                      isScanning ? "กำลังค้นหา..." : "ค้นหาอุปกรณ์",
                      style: const TextStyle(
                          fontSize: 16, fontWeight: FontWeight.bold),
                    ),
                  ),
                ),
                const SizedBox(height: 16),
              ],
            ),
          ),
        ),
      ),
    );
  }

// Helper method to build devices list
  Widget _buildDevicesList(List<ScanResult> devicesList, bool isNamed) {
    if (devicesList.isEmpty) {
      return Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              isNamed ? Icons.bluetooth_searching : Icons.bluetooth_disabled,
              size: 48,
              color: Colors.grey[400],
            ),
            const SizedBox(height: 16),
            Text(
              isNamed
                  ? "ไม่พบอุปกรณ์ที่ระบุชื่อ"
                  : "ไม่พบอุปกรณ์ที่ไม่ระบุชื่อ",
              style: TextStyle(
                color: Colors.grey[600],
                fontSize: 16,
              ),
            ),
          ],
        ),
      );
    }

    return ListView.separated(
      padding: const EdgeInsets.all(16),
      itemCount: devicesList.length,
      separatorBuilder: (context, index) => const Divider(height: 1),
      itemBuilder: (context, index) {
        final device = devicesList[index].device;
        return ListTile(
          contentPadding:
              const EdgeInsets.symmetric(vertical: 8, horizontal: 16),
          leading: Container(
            padding: const EdgeInsets.all(8),
            decoration: BoxDecoration(
              color: isNamed ? Colors.blue.shade50 : Colors.grey.shade100,
              borderRadius: BorderRadius.circular(8),
            ),
            child: Icon(
              isNamed ? Icons.bluetooth : Icons.bluetooth_disabled,
              color: isNamed ? Colors.blue : Colors.grey,
            ),
          ),
          title: Text(
            isNamed
                ? devicesList[index].advertisementData.localName
                : "Unknown Device",
            style: const TextStyle(
              fontWeight: FontWeight.bold,
              fontSize: 16,
            ),
          ),
          subtitle: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const SizedBox(height: 4),
              Text("ID: ${device.id}"),
              Text(
                "Signal: ${devicesList[index].rssi} dBm",
                style: TextStyle(
                  color: _getRssiColor(devicesList[index].rssi),
                ),
              ),
            ],
          ),
          trailing: IconButton(
            icon: const Icon(Icons.link),
            onPressed: () => showConnectDialog(device),
            color: Colors.indigo,
          ),
        );
      },
    );
  }

// Helper method to get RSSI color
  Color _getRssiColor(int rssi) {
    if (rssi >= -60) return Colors.green;
    if (rssi >= -80) return Colors.orange;
    return Colors.red;
  }

  @override
  void dispose() {
    connectedDevice?.disconnect();
    FlutterBluePlus.stopScan();
    super.dispose();
  }
}
