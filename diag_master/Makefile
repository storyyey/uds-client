all:
	gcc -D__HAVE_ZLOG__ -I ../lib_ev/ -I ../diag_master_common/ -I ./libev/ -I ./zlog/ \
		../diag_master_common/cjson.c \
		../diag_master_common/ydm_stream.c \
		../diag_master_common/ydm_ipc_common.c \
		../diag_master_common/yudsc_types.c \
		../diag_master_common/yuds_proto_parse.c \
	   ./zlog/category.c \
	   ./zlog/category_table.c \
	   ./zlog/conf.c \
	   ./zlog/event.c \
	   ./zlog/format.c \
	   ./zlog/level.c \
	   ./zlog/level_list.c \
	   ./zlog/mdc.c \
	   ./zlog/record.c \
	   ./zlog/record_table.c \
	   ./zlog/rotater.c \
	   ./zlog/rule.c \
	   ./zlog/spec.c \
	   ./zlog/thread.c \
	   ./zlog/zc_arraylist.c \
	   ./zlog/zc_hashtable.c \
	   ./zlog/zc_profile.c \
	   ./zlog/zc_util.c \
	   ./zlog/lockfile.c \
	   ./zlog/zlog.c \
	   ./zlog/buf.c \
		./libev/ev.c \
		yuds_client.c \
		ydm.c \
		ydoip_client.c \
		yremote_command.c \
		yshow_services.c \
		main.c \
		-lpthread -lm -o diag_master
clean:
	rm diag_master














