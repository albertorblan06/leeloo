1. Es principalmente un prototipo para una asingatura sobre sistemas empotrados, pero con la idea de ser un producto acabado de lectura para uso diario.

2. EL MVP seria que el ESP32 despliegue la web para que un movil se conecte a ella y suba el pdf, que el movil convierta el pdf a .txt, que el esp32 reciba el .txt y lo renderice correctamente en forma de RSVP.

3. Se puede excluir que reciba el pdf por wifi, y cambiarlo porque cargue un .txt de su propio almacenamiento

4. firmware, app, electronica y estrategia de pruebas (test unitarios, etc). la carcasa la diseñaré yo mismo.

5. Ulegqin ESP32 S3 ESP32-S3-DevKitC-1 con N16R8 Placas Base ESP32 S3 Wroom 1-n16r8 con Interfaces duales Tipo C y 2,4-GHz WLAN Bluetooth 5.0 

6. ram: 8MB, almacenamiento: 16MB

7. choose a sensible default

8. teniamos pensado usar FreeRTOS

9. Antigravity como editor preferido 

10. OLED de 0.96 pulgadas I2C IIC SPI Serial 128x64. Definiciones del pin: VCC: 3,3 V - 5 V. GND: conectar a tierra. SCL: reloj serial. SDA: datos seriales. Modo de comunicación: IIC. Tamaño de la pantalla: 2,4 cm. 

11. Estrictamente I2C

12. En principio preparar principalmente para español castellano

13. choose a sensible default

14. si

15. choose a sensible default

16. EC11

17. choose a sensible default (creo que se pordría configurar en ajustes del dispositivo)

18. pulsación larga en modo wifi interrumpir el modo wifi. pulsacion larga en cualquier otro contexto apagar el dispositivo (deep sleep).

19. la rotacion mientras se reproduce debe subir/bajar la velocidad

20. choose a sensible default (creo que se podria configurar en ajustes del dispositivo)

21. al arrancar mostrar el logo de leeloo (ya lo pondremos, de momento solo mostrar leeloo en text plano). al terminar libro podria poner salir al menu principal o releer. para el resto de casos choose a sensible default. 

22. las ISR solo alimentan tarea de FreeRTOS.

23. rango operativo 150-800 PPM y valor predeterminado 300. el paso de ajuste de 25 en 25.

24. sí, deberian recibir pausa. 

25. palabra como unidad estricta

26. sí, resaltar

27. sí, guardar

28. persistir cuando se salga del libro. (guardar el puntero a la posición exacta del cursor de bytes dentro del archivo de texto).

29. estandarizar todo en el cliente (el navegador web) antes de enviarlo. se codificara en UTF-8 y salto de linea  \n.

30. el cliente convierte y estandariza todo a .txt

31. choose a sensible default

32. actuar como punto de acceso (tipo airdrop). despliega una web donde el cliente sube el archivo, se estandariza y se manda al esp.

33. pin diferente por cada vez que se accede al modo carga wifi

34. navegador moderno, web estetica moderna 

35. choose a sensible default

36. choose a sensible default

37. sí

38. El objetivo del limpiador es conseguir una sola tira continua de palabras separadas por espacios

39. No

40. dar mensaje de error

41. txt con cabecera personalizada en texto plano (titulo, autor, paginas, etc)

42. Eliminar libros sí.

43. pin diferente por cada vez que se accede al modo carga wifi

44. solo emitir wifi cuando este en modo carga wifi

45. choose a sensible default

46. descartado

47. choose a sensible default

48. choose a sensible default (un aviso o similar)

49. sí

50. choose a sensible default

51. de momento no se necesita

52. eliminación para evitar corromper memoria

53. EEMB Batería LiPo 3,7V 500mAh 403048 con JST con APKLVSR TP4056 USBC 5V 1A con protección dual

54. choose a sensible default

55. poder configurar hora en ajustes del dispositivo

56. deep sleep cuando se mantenga pulsado el encoder en el menu principal. cuando se mantiene pulsado durante la lectura o durante pausa de la lectura volver al menu prinipal y ahi ya al mantener pulsado se apaga.

57. encender cuando se mantiene pulsado mientras esta en deep sleep.

58. no decidido todavia, no te ocupes de espacio fisico

59. poder configurar el brillo en los ajustes del dispositivo 

60. no

61. no

62. es un prototipo de momento

63. se conectara el esp32 por usb al mac

64. choose a sensible default

65. choose a sensible default

66. pruebas unitarias de todo lo que se implemente

67. choose a sensible default


