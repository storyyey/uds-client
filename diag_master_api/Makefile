all:
	gcc -c -I ../diag_master_common/ -I./check_algorithm \
		../diag_master_common/cjson.c \
		../diag_master_common/ydm_ipc_common.c \
		../diag_master_common/yuds_proto_parse.c \
		../diag_master_common/yudsc_types.c \
		../diag_master_common/ydm_stream.c \
		yapi_dm_simple.c \
	   ydm_api.c \
	   yapi_dm_shortcut.c \
	   check_algorithm/yapi_crc8.c \
	   check_algorithm/yapi_crc16.c \
	   check_algorithm/yapi_crc32.c \
	   check_algorithm/yapi_crc64.c \
	   check_algorithm/yapi_md5.c
	ar -crv libdm_api.a *.o
clean:
	rm libdm_api.a
	rm *.o














