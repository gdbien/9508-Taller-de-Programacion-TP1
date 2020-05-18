#include "common_protocol.h"
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "common_mem_utils.h"
#include "server.h"
#include "client.h"

//Pre-Header
enum {LITTLE = 'l', BIG = 'b'} endianess = LITTLE;
enum {M_DEFAULT = 1} m_type = M_DEFAULT;
enum {F_DEFAULT = 0} flags = F_DEFAULT;
enum {P_DEFAULT = 1} pro_version = P_DEFAULT;


//Parameters
enum {PATH = 1, DESTINO = 6, INTERFAZ = 2, METODO = 3, FIRMA = 8} par_type;
enum {OBJECT = 'o', STRING = 's', SIGNATURE = 'g'} data_type;

/*
	Swappea los 4 bytes de result con los de aux, en el orden de big a little
	endian.
*/
static void _byte_swapper(char *result, char *aux) {
    result[0] = aux[3];
    result[1] = aux[2];
    result[2] = aux[1];
    result[3] = aux[0];
}

/*
	Convierte hostlong en endiannes del host, al endiannes de la network
	(en este caso para el tp es little endian).
	Devuelve el hostlong convertido.
*/
static uint32_t _htonl(uint32_t hostlong) {
    uint32_t result = htonl(hostlong);
    uint32_t aux = htonl(hostlong);
    _byte_swapper((char*) &result, (char*) &aux);
    return result;
}

/*
	Convierte hostlong en endiannes de la network, al endiannes del host.
	Devuelve el hostlong convertido.
*/
static uint32_t _ntohl(uint32_t netlong) {
    return (ntohl(htonl(netlong)));
}

/*
	Crea un array de padding de amount bytes, modificando 
	*array.
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
static int _create_pad_array(char** array, size_t amount) {
	char *result = realloc(*array, amount);
    if (!result) {
    	free(*array);
		return ERROR;
	}
	memset(result, 0, amount);
	*array = result;
	return SUCCESS;
}

static void _set_header_pre(header_pre_t *header_pre, int8_t endianess,
						    int8_t m_type, int8_t flags, int8_t pro_version,
						    int32_t body_size, int32_t arr_size) {
	static int message_id = 1;
	header_pre->endianess = endianess;
	header_pre->m_type = m_type;
	header_pre->flags = flags;
	header_pre->pro_version = pro_version;
	header_pre->body_size = body_size;
	header_pre->message_id = message_id;
	header_pre->arr_size = arr_size;
	message_id++;
}

/*
	Devuelve el multiplo de 8 mayor mas cercano a number.
*/
int next_multiple_8(size_t number) {
	return (number + (8 - 1)) & ~(8 - 1); 
}

static void _set_base_param(base_param_t *base_param, int8_t par_type,
						    int8_t byte_1, int8_t data_type, int8_t null_byte) {
	base_param->par_type = par_type;
	base_param->byte_1 = byte_1;
	base_param->data_type = data_type;
	base_param->null_byte = null_byte;
}

static void _set_norm_param(norm_param_t *norm_param, int8_t par_type,
						    int8_t byte_1, int8_t data_type, int8_t null_byte,
						    int32_t data_size) {
	base_param_t base_param;
	_set_base_param(&base_param, par_type, byte_1, data_type, null_byte);
	norm_param->base_param = base_param;
	norm_param->data_size = data_size;
}

static void _set_sign_param(sign_param_t *sign_param, int8_t par_type,
							int8_t byte_1, int8_t data_type, int8_t null_byte,
						    int8_t arg_count) {
	base_param_t base_param;
	_set_base_param(&base_param, par_type, byte_1, data_type, null_byte);
	sign_param->base_param = base_param;
	sign_param->arg_count = arg_count;	
}

/*
	Devuelve el par_type y data_type, asociado al orden de ENCODE
	decidido por el usuario (para decodificar el enunciado decia
	que podian estar en cualquier orden, no necesariamente en este orden).
	Hay que acordarse de resetear array_order cuando se tiene un nuevo llamado
	de función (nueva línea) lo cual se hace llamando una vez mas
	a _get_par_type_order().

*/
static void _get_par_type_order() {
	static int array_order = 0;
	switch (array_order) {
		case 0:
			par_type = DESTINO;
			data_type = STRING;
			break;
		case 1:
			par_type = PATH;
			data_type = OBJECT;
			break;
		case 2:
			par_type = INTERFAZ;
			data_type = STRING;
			break;
		case 3:
			par_type = METODO;
			data_type = STRING;
			break;
		case 4:
			//Reset
			array_order = 0;
			return;
	}
	array_order++;
}

/*
	Se encarga de verificar y agregar padding a param_arr (si es necesario)
*/
static int _prot_enc_add_pad(char **param_arr, int32_t *pa_arr_size,
							 char **pad_arr) {
	size_t n_multiple_8;
	size_t pad_bytes;
	n_multiple_8 = next_multiple_8(*pa_arr_size); 
	if (*pa_arr_size != n_multiple_8) {			
		pad_bytes = n_multiple_8 - *pa_arr_size;
		_create_pad_array(pad_arr, pad_bytes);
		*param_arr = array_concat(*param_arr, *pa_arr_size, *pad_arr,
								  pad_bytes, pa_arr_size);
	}
	return SUCCESS;
}

/*
	Se encarga de crear un norm_param_t, setearlo dependiendo de que tipo
	de parametro se trate, y lo agrega a param_arr junto con el nombre del 
	parametro correspondiente.
	Delega el agregado de padding.
*/
static int _prot_enc_proc_norm_param(char **param_arr, int32_t *pa_arr_size,
 									 char *param_name, char **pad_arr,
 									 int32_t *arr_size) {
	_get_par_type_order();
	norm_param_t norm_param;
	_set_norm_param(&norm_param, par_type, 1, data_type, '\0',
				    _htonl(strlen(param_name)));
	*param_arr = array_concat(*param_arr, *pa_arr_size, (char*)&norm_param,
							  sizeof(norm_param_t), pa_arr_size);
	*param_arr = array_concat(*param_arr, *pa_arr_size, param_name,
							  strlen(param_name) + 1, pa_arr_size);
	*arr_size = *pa_arr_size;
	_prot_enc_add_pad(param_arr, pa_arr_size, pad_arr);
	return SUCCESS;
}

/*
	Se encarga de crear un sign_param_t junto, setearlo dependiendo de que tipo
	de parametro se trate, y lo agrega a param_arr junto con el string
	compuesto por 'sss' con su cantidad correspondiente.
	Delega el agregado de padding.
*/
static int _prot_enc_proc_sign_param(char **param_arr, int32_t *pa_arr_size,
 									 size_t n_arg, char **pad_arr,
 									 int32_t *arr_size) {
	sign_param_t sign_param;
	par_type = FIRMA;
	data_type = SIGNATURE;
	_set_sign_param(&sign_param, par_type, 1, data_type, '\0', n_arg);
	char * firma_s_arr = malloc(n_arg * sizeof(char) + 1);
	memset(firma_s_arr,'s',n_arg);
	*(firma_s_arr + n_arg) = '\0';
	*param_arr = array_concat(*param_arr, *pa_arr_size, (char*)&sign_param,
							  sizeof(sign_param_t), pa_arr_size);
	*param_arr = array_concat(*param_arr, *pa_arr_size, firma_s_arr,
							  strlen(firma_s_arr) + 1, pa_arr_size);
	free(firma_s_arr);
	*arr_size = *pa_arr_size;  //Me guardo la longitud antes de agregar padding
	_prot_enc_add_pad(param_arr, pa_arr_size, pad_arr);
	return SUCCESS;
}


/*
	Se encarga de crear el body
*/
static int _prot_enc_add_arg(char **body_arr, int32_t *body_size,
							 char *arg_names[], size_t n_arg) {
	int32_t arg_length;
	for (size_t i = 0; i < n_arg; i++) {
		arg_length = strlen(arg_names[i]);
		arg_length = _htonl(arg_length);
		*body_arr = array_concat(*body_arr, *body_size, (char*) &arg_length,
								 4, body_size);
		*body_arr = array_concat(*body_arr, *body_size, arg_names[i],
								 strlen(arg_names[i]) + 1, body_size);
	}
	return SUCCESS;
}


/*
	Realiza la concatenacion final, agregando primero el header_pre, luego el
	arreglo de parametros, y por último el body, todos previamente creados.
*/
static int _prot_enc_pack_data(char **data, size_t *encoded_size,
							   header_pre_t *header_pre, char *param_arr,
							   int32_t pa_arr_size, size_t n_arg,
							   char *body_arr, int32_t body_size) {
	*data = array_concat(*data, *encoded_size, (char*)header_pre,
						 sizeof(header_pre_t), (int32_t*)encoded_size);
	*data = array_concat(*data, *encoded_size, param_arr, pa_arr_size,
					    (int32_t*)encoded_size);
	if (n_arg > 0) *data = array_concat(*data, *encoded_size, body_arr,
									    body_size, (int32_t*)encoded_size);
	return SUCCESS;
}

/*
	Recibe un message_t para que se le aplique el protocolo,
	y devuelva el llamado encodeado en data (reservando memoria dinámica
	para él).
	Debe ser llamado inicialmente con *data = NULL, y el usuario debe
	encargarse de liberar la memoria.
	Devuelve la cantidad de bytes que ocupa el mensaje encodeado,
	ERROR en caso de error.
*/
static int _protocol_encode(char **data, message_t *message) {
	int32_t arr_size; //Para header_pre
	int32_t body_size = 0; //Para header_pre
	char *pad_arr = NULL;
	char *body_arr = NULL;
	size_t encoded_size = 0;

	char *param_arr = NULL;
	int32_t pa_arr_size = 0;

	_prot_enc_proc_norm_param(&param_arr, &pa_arr_size, message->destination,
							  &pad_arr, &arr_size);
	_prot_enc_proc_norm_param(&param_arr, &pa_arr_size, message->path,
							  &pad_arr, &arr_size);
	_prot_enc_proc_norm_param(&param_arr, &pa_arr_size, message->interface,
							  &pad_arr, &arr_size);
	_prot_enc_proc_norm_param(&param_arr, &pa_arr_size, message->method,
							  &pad_arr, &arr_size);

	size_t n_args = message->n_args;
	if (n_args > 0) { 	//Tengo parametro firma y body!
		_prot_enc_proc_sign_param(&param_arr, &pa_arr_size, n_args, &pad_arr,
								  &arr_size);
		_prot_enc_add_arg(&body_arr, &body_size, message->arguments, n_args);
	}

	header_pre_t header_pre;
	_set_header_pre(&header_pre, endianess, m_type, flags, pro_version,
				    _htonl(body_size), _htonl(arr_size));
	_prot_enc_pack_data(data, &encoded_size, &header_pre, param_arr,
					    pa_arr_size, n_args, body_arr, body_size);
	free(param_arr);
	free(body_arr);
	free(pad_arr);
	_get_par_type_order(); //Para resetear el orden.
	return encoded_size;
}

int protocol_send(prot_cb_t cb_send, message_t *message, void *context) {
	client_t *client = (client_t*) context;
	char *encoded_data = NULL;
	int encoded_size = _protocol_encode(&encoded_data, message);
	if (encoded_size < 0) return ERROR;
	int count = cb_send(client, encoded_data, encoded_size);
	if (count < 0) {
		free(encoded_data);
		return ERROR;
	} 
	free(encoded_data);
	return encoded_size;
}

/*
	Procesa el string asociado a un parametro normal, almacenando memoria
	dinámica para cada uno en param_names, modificiando la posición de
	arr_idx en el array.
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
static int _prot_dec_proc_norm_param(const char *encoded_arr, size_t* arr_idx,
								     char **param_name) {
	int32_t data_size;
	memcpy(&data_size, encoded_arr + *arr_idx, sizeof(int32_t));
	data_size = _ntohl(data_size);
	*arr_idx += sizeof(int32_t);
	*param_name = malloc(data_size * sizeof(char) + 1);
	if (!*param_name) return ERROR;
	memcpy(*param_name, encoded_arr + *arr_idx, data_size + 1);
	*arr_idx += data_size + 1;
	return SUCCESS;
}

/*
	Decodifica los parámetros, almacenando sus nombres en param_names,
	y si tiene parámetro firma, modifica la cantidad de argumentos (n_arg).
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
static int _protocol_decode_parameters(const char *encoded_arr, size_t length,
							    char *param_names[4], size_t *n_arg) {
	int n_multiple_8;
	base_param_t base_param;
	int8_t arg_count;
	size_t arr_idx = 0;
	int ret;
	while (arr_idx < length) {
		memcpy(&base_param, encoded_arr + arr_idx, sizeof(base_param_t));
		arr_idx += sizeof(base_param_t);
		par_type = base_param.par_type;
		switch (par_type) {
			case DESTINO:
				ret = _prot_dec_proc_norm_param(encoded_arr, &arr_idx,
											    &param_names[0]);
				if (ret == ERROR) return ERROR;
				break;
			case PATH:
				ret = _prot_dec_proc_norm_param(encoded_arr, &arr_idx,
											    &param_names[1]);
				if (ret == ERROR) return ERROR;
				break;
			case INTERFAZ:
				ret = _prot_dec_proc_norm_param(encoded_arr, &arr_idx,
											    &param_names[2]);
				if (ret == ERROR) return ERROR;
				break;
			case METODO:
				ret = _prot_dec_proc_norm_param(encoded_arr, &arr_idx,
											    &param_names[3]);
				if (ret == ERROR) return ERROR;
				break;
			case FIRMA:
				memcpy(&arg_count, encoded_arr + arr_idx, sizeof(int8_t));
				*n_arg = arg_count;
				arr_idx += sizeof(int8_t);
				arr_idx += arg_count + 1;
				break;
			default:
				printf("ERROR! 404 NOT FOUND\n");
				break;
		}
		n_multiple_8 = next_multiple_8(arr_idx);
		if (arr_idx != n_multiple_8) {
			arr_idx += n_multiple_8 - arr_idx;
		}
	}
	return SUCCESS;
}

/*
	Decodifica los argumentos, almacenando sus nombres en arg_names.
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
static int _protocol_decode_arguments(const char *encoded_body,
									  char *arg_names[], size_t n_arg) {
	size_t idx = 0;
	int32_t string_size;
	for (size_t i = 0; i < n_arg; i++) {
		memcpy(&string_size, encoded_body + idx, sizeof(int32_t));
		string_size = _ntohl(string_size);
		idx += sizeof(int32_t);
		arg_names[i] = malloc(string_size * sizeof(char) + 1);
		if (!arg_names[i]) return ERROR;
		memcpy(arg_names[i], encoded_body + idx, string_size + 1);
		idx += string_size + 1;
	}
	return SUCCESS;
}

/*
	Se encarga de recibir el header_pre del mensaje encodeado.
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
static int _prot_rec_header_pre(prot_cb_t cb_receive, server_t *server,
					    header_pre_t* header_pre) {
	int count = cb_receive(server, (char*) header_pre, sizeof(header_pre_t));
	if (count <= 0) return ERROR;
	return SUCCESS;
}

/*
	Se encarga de recibir array de parámetros del mensaje encodeado en buffer.
	Reserva memoria dinámica para buffer, que luego el usuario debe encargarse
	de liberarla.
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
static int _prot_rec_arr_no_pad(prot_cb_t cb_receive, server_t *server,
							    char **buffer, size_t arr_size) {
	*buffer = malloc(arr_size * sizeof(char));
	int count = cb_receive(server, *buffer, arr_size);
	if (count < 0) return ERROR;
	return SUCCESS;
}

/*
	Se encarga de recibir el padding excediente al final del array de
	parametros en buffer.
	Hace realloc del buffer que estaba previamente en el heap.
	El usuario debe encargarse de librar la memoria al final. 
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
static int _prot_rec_padding(prot_cb_t cb_receive, server_t *server,
							 char **buffer, size_t arr_size) {
	size_t last_padding = 0;
	int n_multiple_8 = next_multiple_8(arr_size);
	if (arr_size != n_multiple_8) {
		last_padding = n_multiple_8 - arr_size;
	}
	*buffer = realloc(*buffer, last_padding * sizeof(char));
	int count = cb_receive(server, *buffer, last_padding);
	if (count < 0) return ERROR;
	return SUCCESS;
}

/*
	Se encarga de recibir el body que contiene los argumentos del método en
	en buffer.
	Realloca el buffer que previamente estaba en el heap.
	Devuelve SUCCESS si ok, ERROR en caso contrario.
*/
static int _prot_rec_body(prot_cb_t cb_receive, server_t *server, char **buffer,
				   size_t body_size) {
	*buffer = realloc(*buffer, body_size * sizeof(char));
	int count = cb_receive(server, *buffer, body_size);
	if (count < 0) return ERROR;
	return SUCCESS;
}


int protocol_receive(prot_cb_t cb_receive, message_t *message, void *context) {
	server_t *server = (server_t*) context;
	int ret;
	size_t n_arg = 0;
	size_t arr_size;
	header_pre_t header_pre;
	char *buffer = NULL;
	char *param_names[4];
	char **arg_names = NULL;
	ret = _prot_rec_header_pre(cb_receive, server, &header_pre);
	if (ret < 0) return ERROR;
	arr_size = _ntohl(header_pre.arr_size);
	_prot_rec_arr_no_pad(cb_receive, server, &buffer, arr_size);
	_protocol_decode_parameters(buffer, arr_size, param_names, &n_arg);
	_prot_rec_padding(cb_receive, server, &buffer, arr_size);
	size_t body_size = _ntohl(header_pre.body_size);
	if (body_size != 0)	{
		arg_names = malloc(n_arg * sizeof(char*));
		_prot_rec_body(cb_receive, server, &buffer, body_size);
		_protocol_decode_arguments(buffer, arg_names, n_arg);
	}
	message_setter(message, header_pre.message_id, param_names[0],
				   param_names[1], param_names[2], param_names[3],
				   n_arg, arg_names);
	free(buffer);
	return SUCCESS;
}



