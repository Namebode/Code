import 'package:flutter/material.dart';
import 'ble_scan_page.dart';

class HomePage extends StatelessWidget {
  const HomePage({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      extendBodyBehindAppBar: true,
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        title: const Text(
          "BLE Device Setup",
          style: TextStyle(
            color: Colors.white,
            fontWeight: FontWeight.bold,
            fontSize: 22,
          ),
        ),
        centerTitle: true,
      ),
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            colors: [
              Color.fromRGBO(178, 150, 255, 1), // ใช้โทนสีม่วงเดิม
              Color.fromARGB(255, 210, 164, 248), // ใช้โทนสีม่วงเดิม
            ],
            begin: Alignment.topCenter, // ปรับทิศทาง Gradient
            end: Alignment.bottomCenter, // ปรับทิศทาง Gradient
            stops: [0.3, 0.7], // ปรับสัดส่วนของสีใน Gradient
          ),
        ),
        child: Center(
          child: Padding(
            padding: const EdgeInsets.symmetric(horizontal: 32.0),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Container(
                  padding: const EdgeInsets.all(24),
                  decoration: BoxDecoration(
                    color: Colors.white.withOpacity(0.2),
                    shape: BoxShape.circle,
                    boxShadow: [
                      BoxShadow(
                        color: Colors.black.withOpacity(0.1),
                        blurRadius: 10,
                        spreadRadius: 2,
                      )
                    ],
                  ),
                  child: const Icon(
                    Icons.bluetooth_searching,
                    size: 100,
                    color: Colors.white,
                  ),
                ),
                const SizedBox(height: 48),
                Text(
                  "ค้นหาและตั้งค่าอุปกรณ์ BLE",
                  textAlign: TextAlign.center,
                  style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                        color: Colors.white,
                        fontWeight: FontWeight.bold,
                      ),
                ),
                const SizedBox(height: 24),
                Text(
                  "แตะปุ่มด้านล่างเพื่อเริ่มสแกน\nและค้นหาอุปกรณ์ BLE ใกล้เคียง",
                  textAlign: TextAlign.center,
                  style: Theme.of(context).textTheme.bodyLarge?.copyWith(
                        color: Colors.white70,
                        height: 1.5,
                      ),
                ),
                const SizedBox(height: 60),
                ElevatedButton(
                  onPressed: () {
                    Navigator.push(
                      context,
                      MaterialPageRoute(
                          builder: (context) => const BLEScanPage()),
                    );
                  },
                  style: ElevatedButton.styleFrom(
                    padding: const EdgeInsets.symmetric(
                        vertical: 18, horizontal: 50),
                    backgroundColor: Colors.white,
                    foregroundColor: const Color.fromRGBO(
                        178, 150, 255, 1), // ใช้สีม่วงจาก Gradient
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(30),
                    ),
                    elevation: 10,
                    shadowColor: Colors.black.withOpacity(0.3),
                  ),
                  child: const Text(
                    "ไปหน้าสแกนBLUETOOTH",
                    style: TextStyle(
                      fontSize: 18,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
