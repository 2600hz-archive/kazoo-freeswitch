/*
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2014, Anthony Minessale II <anthm@freeswitch.org>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Anthony Minessale II <anthm@freeswitch.org>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * James Aimonetti <james@2600hz.com>
 *
 * mod_trace.c -- Framework Demo Module
 *
 */
#include <switch.h>

/* Prototypes */
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_trace_shutdown);
SWITCH_MODULE_RUNTIME_FUNCTION(mod_trace_runtime);
SWITCH_MODULE_LOAD_FUNCTION(mod_trace_load);

typedef enum {
  EVENT_FORMAT_PLAIN,
  EVENT_FORMAT_XML,
  EVENT_FORMAT_JSON
} event_format_t;

static struct {
  switch_event_node_t *node;
  event_format_t format;
  switch_memory_pool_t *pool;
  switch_mutex_t *mutex;
  switch_file_t *fd;
} globals;

static void write_to_file(switch_event_t *event, char* buf) {
  switch_size_t
    buf_len = strlen(buf),
    eol = 1;

  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "grabbing mutex\n");
  switch_mutex_lock(globals.mutex);

  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "writing to file\n");

  if( (switch_file_write(globals.fd, buf, &buf_len)) == SWITCH_STATUS_SUCCESS ) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "writing eol\n");
    switch_file_write(globals.fd, "\n", &eol);
  } else {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "failed to write buffer\n");
  }

  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "unlock the mutex\n");
  switch_mutex_unlock(globals.mutex);

  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "return void\n");
  return;
}

static void trace_handler(switch_event_t *event)
{
  char
    *trace_event = NULL,
    *buf = NULL;

  if ( !(trace_event = switch_event_get_header(event, "variable_trace_event")) || switch_false(trace_event) ) {
    return;
  }

  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "tracing event %s\n", switch_event_name(event->event_id));

  switch(globals.format) {
  case EVENT_FORMAT_PLAIN:
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "attempting to write event %s as PLAIN to file\n", switch_event_name(event->event_id));
    break;
  case EVENT_FORMAT_XML:
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "attempting to write event %s as XML to file\n", switch_event_name(event->event_id));
    break;
  case EVENT_FORMAT_JSON:
  default:
    if (switch_event_serialize_json(event, &buf) == SWITCH_STATUS_SUCCESS) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "attempting to write event %s as JSON to file\n", switch_event_name(event->event_id));
      write_to_file(event, buf);
    } else {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "failed to serialize event %s\n", switch_event_name(event->event_id));
    }
    break;
  }

  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "free buf\n");
  switch_safe_free(buf);

  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "free trace_event\n");
  switch_safe_free(trace_event);

  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "done with this event\n");

  return;
}

/* SWITCH_MODULE_DEFINITION(name, load, shutdown, runtime)
 * Defines a switch_loadable_module_function_table_t and a static const char[] modname
 */
SWITCH_MODULE_DEFINITION(mod_trace, mod_trace_load, mod_trace_shutdown, NULL);

/* Macro expands to: switch_status_t mod_trace_load(switch_loadable_module_interface_t **module_interface, switch_memory_pool_t *pool) */
SWITCH_MODULE_LOAD_FUNCTION(mod_trace_load)
{
  char *path = switch_mprintf("%s%s%s", "/tmp", SWITCH_PATH_SEPARATOR, "trace.dat");
  unsigned int flags = 0;

  /* connect my internal structure to the blank pointer passed to me */
  *module_interface = switch_loadable_module_create_module_interface(pool, modname);

  memset(&globals, 0, sizeof(globals));

  globals.format = EVENT_FORMAT_JSON;
  globals.pool = pool;

  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Initializing mutex!\n");
  switch_mutex_init(&globals.mutex, SWITCH_MUTEX_NESTED, globals.pool);

  if (switch_event_bind_removable(modname, SWITCH_EVENT_ALL, SWITCH_EVENT_SUBCLASS_ANY, trace_handler, NULL, &globals.node) != SWITCH_STATUS_SUCCESS) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind!\n");
    return SWITCH_STATUS_GENERR;
  }

  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Bound for events!\n");

  flags |= SWITCH_FOPEN_WRITE;
  flags |= SWITCH_FOPEN_CREATE;
  flags |= SWITCH_FOPEN_APPEND;

  if( switch_file_open(&globals.fd
                       ,path
                       ,flags
                       ,SWITCH_FPROT_OS_DEFAULT
                       ,globals.pool
                      )
      != SWITCH_STATUS_SUCCESS
      ) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "failed to open path %s\n", path);
    return SWITCH_STATUS_GENERR;
  }

  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "File is open for writing!\n");

  /* indicate that the module should continue to be loaded */
  return SWITCH_STATUS_SUCCESS;
}

/*
  Called when the system shuts down
  Macro expands to: switch_status_t mod_trace_shutdown()
*/
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_trace_shutdown)
{
  switch_event_unbind(&globals.node);

  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "close the file\n");
  switch_file_close(globals.fd);
  globals.fd = NULL;

  return SWITCH_STATUS_SUCCESS;
}
