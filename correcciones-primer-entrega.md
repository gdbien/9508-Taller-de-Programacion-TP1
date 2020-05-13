El tp no está mal, pero hay mucha cohesión entre los métodos.
Es muy dificil hacer el seguimiento del protocolo, tiene mucho manejo de memoria dinámica desperdigado.
A la función `_strdup` no le veo un uso práctico, está generando mucha información en el heap. Mucho menos a `array_concat`.
Las funciones privadas si bien son privadas, tendrías que ponerle nombres descriptivos, o al menos una documentación mínima.
Es confuso que en el servidor manejes los parámetros como un array de char"y por otro lado tengas los tdas del protocolo, por lo que entiendo ahí hay información redundante.
Reformularía la API de protocol a algo como lo siguiente:
`protocol_encode(buffer**, message_t*)` Donde `message_t` es un tda con todos los atributos del header y body ya parseados
`protocol_decode(buffer**, message_t*)`

Entonces el cliente convierte una linea de texto a un message_t, y el protocolo lo codifica en un buffer (es más, si en vez de forzar el heap le das la opción de saber el largo del buffer para pasarle un buffer prealocado, mejor)
El servidor hace la operación inversa, de un buffer rearma message_t. Es más claro y te permite recibir mensajes con los parámetros desordenados (que creo que en este momento no lo puede hacer)

Ojo con el uso de packed: https://stackoverflow.com/questions/8568432/is-gccs-attribute-packed-pragma-pack-unsafe
