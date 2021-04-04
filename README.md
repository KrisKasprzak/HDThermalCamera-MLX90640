<b><h2><center>Version 1.0 updated 4/4/2021</center></h1></b>

<b><h2><center>Sample Program for displaying MLX90640</center></h2></b>

This program is the full source code in displaying temperatures measured from an MLX90640 thermal camera. The camera is a 32 x 24 sensor array, but this program will scale the display up to 310 x 230 through interpolation. Color mapping between temperatures and a 565 color palet is used for displaying data. Other features are legends with a temperature gradient and a histogram showing temperatur frequencies. Other controls let the user set the temperatue, range, and refresh rate.

This porgram was tested using a Teensy 4.0 and ILI9341-based display. It is highly unlikely this program can be ported to use an Arduino, as they will probably not have enough memory or computing power.

<br>
<br>

<b><h2>Simple display of 32 x 24 1:1 pixel sizing</h2></b><br>
![header image](https://raw.github.com/KrisKasprzak/HDThermalCamera-MLX90640/master/01px.jpg)

<b><h2>Scaled display to 320 x 240 1:10 pixel sizing</h2></b><br>
![header image](https://raw.github.com/KrisKasprzak/HDThermalCamera-MLX90640/master/10px.jpg)

<b><h2>Scaled display to 310 x 230 1:10 pixel with interpolation</h2></b><br>
![header image](https://raw.github.com/KrisKasprzak/HDThermalCamera-MLX90640/master/ThermalCamera.jpg)


<br>
<br>

