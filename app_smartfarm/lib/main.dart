import 'package:flutter/material.dart';
import 'homepage.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'BLE Device Config',
      theme: ThemeData(
        primarySwatch: MaterialColor(
          0xFF9C84D6, // สีม่วงพาร์เทล (Pastel Purple)
          <int, Color>{
            50: const Color(0xFFF4ECFC),
            100: const Color(0xFFE2C6F7),
            200: const Color(0xFFCE9BF3),
            300: const Color(0xFFB773EF),
            400: const Color(0xFFA05FEA),
            500: const Color(0xFF9C84D6), // สีหลัก
            600: const Color(0xFF8A6CC4),
            700: const Color(0xFF7558B1),
            800: const Color(0xFF604B9E),
            900: const Color(0xFF4B3F8B),
          },
        ),
        scaffoldBackgroundColor: const Color(0xFFF9F2FF),
        textTheme: TextTheme(
          bodyLarge: TextStyle(color: Colors.black.withOpacity(0.8)),
          bodyMedium: TextStyle(color: Colors.black.withOpacity(0.8)),
          titleLarge: TextStyle(color: const Color(0xFF9C84D6)),
        ),
        appBarTheme: AppBarTheme(
          color: const Color(0xFF9C84D6),
        ),
      ),
      home: const SplashScreen(),
    );
  }
}

class SplashScreen extends StatefulWidget {
  const SplashScreen({Key? key}) : super(key: key);

  @override
  _SplashScreenState createState() => _SplashScreenState();
}

class _SplashScreenState extends State<SplashScreen> {
  @override
  void initState() {
    super.initState();
    Future.delayed(const Duration(seconds: 3), () {
      Navigator.pushReplacement(
        context,
        MaterialPageRoute(builder: (context) => const HomePage()),
      );
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color.fromARGB(255, 196, 172, 196),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            // ใส่ภาพที่โหลดจาก assets
            Image.asset(
              'assets/icon/grape.png', // เพิ่มชื่อไฟล์ภาพใน assets
              width: 150, // ขนาดของภาพ
            ),
            const SizedBox(height: 20),
            const CircularProgressIndicator(
                color: Color.fromARGB(255, 0, 0, 0)), // สีม่วงพาร์เทล
            const SizedBox(height: 20),
            const Text(
              'Loading...',
              style:
                  TextStyle(fontSize: 24, color: Color.fromARGB(255, 0, 0, 0)),
            ),
          ],
        ),
      ),
    );
  }
}
