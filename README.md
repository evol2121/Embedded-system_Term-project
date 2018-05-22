# Embedded-system_Term-project
my smartfarm 

demo youtube link: https://youtu.be/9Jc7R8Y3HOM

Monitor temperature and lightness on every 1ms
Use an analogous data for a ligthness sensor. 
Build a server in cloud and install MySQL and Apache into it. 
(you can use any sort of a cloud service or use your desktop as a server)
Send sensor data from Raspberry Pi to the server every 10s. 
Turn on FAN when the temperature goes beyond 28 degrees (C)  for 5 seconds. 
Turn off LED when the lightness goes below  140 and turn on it otherwise. 
Each functionality should be performed by an independant thread (use multi-threaded programming, 4 threads) 
Use a signal mechanism when threads need to communicate each other. 

스레드를 활용하여 제작한 smartfarm입니다. 1ms마다 온도와 조도를 체크하고 온도가 28이상일때 팬을 5초동안 동작시키고 조도가 어두워지면(140기준) LED를 키고 밝아지면 LED를 끕니다. 10초마다 DB에 값을 저장시키고, 이는 apach2와 php를 통해 확인할 수 있습니다. 

