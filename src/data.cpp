#include <app_common.h>
#include <Elementary.h>
#include <efl_extension.h>
#include <dlog.h>
#include <omahawatch.h>

#include "data.h"

void data_get_resource_path(const char *file_in, char *file_path_out, int file_path_max)
{
	char *res_path = app_get_resource_path();
	if (res_path) {
		snprintf(file_path_out, file_path_max, "%s%s", res_path, file_in);
		free(res_path);
	}
}

