#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include <katcp.h>
#include <katcl.h>
#include <katpriv.h>
#include <kcs.h>

#define KATCP_OPERATION_ROACH_CONNECT "roachconnect"
#define KATCP_EDGE_ROACH_PING         "ping"
#define KATCP_TYPE_ROACH              "roach"
#define KATCP_TYPE_URL                "url"

void print_katcp_url_type_mod(struct katcp_dispatch *d, void *data)
{
  struct katcp_url *ku;
  ku = data;
  if (ku == NULL)
    return;

  append_string_katcp(d, KATCP_FLAG_STRING | KATCP_FLAG_FIRST, "#katcp url:");
  append_string_katcp(d, KATCP_FLAG_STRING, ku->u_str);
  append_string_katcp(d, KATCP_FLAG_STRING, "use count:");
  append_unsigned_long_katcp(d, KATCP_FLAG_ULONG | KATCP_FLAG_LAST, ku->u_use);
}
void destroy_katcp_url_type_mod(void *data)
{
  struct katcp_url *ku;
  ku = data;
  destroy_kurl_katcp(ku);
}
void *parse_katcp_url_type_mod(char **str)
{
  struct katcp_url *ku;
  ku = create_kurl_from_string_katcp(str[0]);
  if (ku == NULL)
    return NULL;
  ku->u_use++;
  return ku;
}

#if 0
struct katcp_roach {
  struct katcp_url *r_url;
  struct katcp_job *r_job;
  struct katcp_notice *r_statemachine_notice;
  struct katcp_type **r_tags;
  struct timeval r_seen;
  int r_refs;
};

void print_roach_type_mod(struct katcp_dispatch *d, void *data)
{
  struct katcp_roach *r;
  struct katcp_url *ku;

  r = data;
  if (r == NULL)
    return;

  ku = r->r_url;
  if (ku == NULL)
    return;

#ifdef DEBUG
  fprintf(stderr, "mod_roach_comms: print_roach_type %s\n", ku->u_str);
#endif
  //log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "kat url: %s", ku->u_str);
 // prepend_inform_katcp(d);
  append_string_katcp(d, KATCP_FLAG_STRING | KATCP_FLAG_FIRST, "#roach url:");
  append_string_katcp(d, KATCP_FLAG_STRING, ku->u_str);
  append_string_katcp(d, KATCP_FLAG_STRING, "use count:");
  append_unsigned_long_katcp(d, KATCP_FLAG_ULONG | KATCP_FLAG_LAST, ku->u_use);
}
void destroy_roach_type_mod(void *data)
{
  struct katcp_roach *r;
  struct katcp_url *ku;

  r = data;
  if (r == NULL)
    return;

  ku = r->r_url;
  
  if (ku != NULL)
#ifdef DEBUG
    fprintf(stderr, "mod_roach_comms: del roach url: %s\n", ku->u_str);
#endif
  
  destroy_kurl_katcp(ku);
  free(r->r_tags);
  
  r->r_job = NULL;
  r->r_statemachine_notice = NULL;

  free(r);
}
void *parse_roach_type_mod(char **str)
{ 
  struct katcp_roach *r;
  struct katcp_url *ku;
  
  ku = create_kurl_from_string_katcp(str[0]);
  if (ku == NULL)
    return NULL;
  
  r = malloc(sizeof(struct katcp_roach));
  if (r == NULL){
    destroy_kurl_katcp(ku);
    return NULL;
  }

  ku->u_use++;
  
  r->r_url  = ku;
  r->r_tags = NULL;
  r->r_job  = NULL;
  r->r_statemachine_notice = NULL;

  return r;
}
#endif

int roach_disconnect_mod(struct katcp_dispatch *d, struct katcp_notice *n, void *data)
{
#if 0
  struct katcp_roach *r;
  struct katcp_url *u;
  
  r = data;
  if (r == NULL)
    return 0;

  u = r->r_url;
  if (u == NULL)
    return 0;

  //del_data_type_katcp(d, KATCP_TYPE_ROACH, u->u_str);

  log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "roach comms: notice disconnect %s", u->u_str);
  
#endif
  return 0;
}

int roach_connect_mod(struct katcp_dispatch *d, struct kcs_sm_state *s, struct katcp_stack_obj *o)
{
  struct katcp_stack *stack;
  struct katcp_url *u;
  struct katcp_notice *n;
  struct katcp_job *j;
  struct katcp_actor *a;

  stack = get_sm_stack_kcs(s);

  u = pop_data_expecting_stack_katcp(d, stack, KATCP_TYPE_URL);
  if (u == NULL)
    return -1;
  
  n = register_notice_katcp(d, NULL, 0, &roach_disconnect_mod, NULL);
  if (n == NULL){
    return -1;
  }
  
  j = network_connect_job_katcp(d, u, n);
  if (j == NULL){
    return -1;
  }
  
  a = create_actor_type_katcp(d, u->u_str, j, NULL, NULL, NULL);
  if (a == NULL){
    zap_job_katcp(d, j);
    return -1;
  }
 
  if (store_data_type_katcp(d, KATCP_TYPE_ACTOR, KATCP_DEP_BASE, u->u_str, a, &print_actor_type_katcp, &destroy_actor_type_katcp, &copy_actor_type_katcp, &compare_actor_type_katcp, &parse_actor_type_katcp) < 0){
    zap_job_katcp(d, j);
    destroy_actor_type_katcp(a);
    return -1;
  }
    
  log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "roach connected %s with job %p", u->u_str, j);

  return 0;
}

struct kcs_sm_op *roach_connect_setup_mod(struct katcp_dispatch *d, struct kcs_sm_state *s)
{
  struct kcs_sm_op *op;

  op = create_sm_op_kcs(&roach_connect_mod, NULL);
  if (op == NULL)
    return NULL;

#ifdef DEBUG
  fprintf(stderr, "mod_roach_comms: created op %s (%p)\n", KATCP_OPERATION_ROACH_CONNECT, op);
#endif

  return op;
}

int roach_ping_returns_mod(struct katcp_dispatch *d, struct katcp_notice *n, void *data)
{
#if 0
  struct katcp_roach *r;
  struct katcl_parse *p;
  int i, max;
  char *ptr;
  struct timeval now, delta;
 
  r = data;
  if (r == NULL)
    return 0;
 
  gettimeofday(&now, NULL);

  p = get_parse_notice_katcp(d, n);
  if (p){
    max = get_count_parse_katcl(p);
    for (i=0; i<max; i++){
      ptr = get_string_parse_katcl(p, i);
      log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "%s", ptr);
    }
    sub_time_katcp(&delta, &now, &r->r_seen);
    log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "%s reply in %4.3fms", r->r_url->u_str, (float)(delta.tv_sec*1000)+((float)delta.tv_usec/1000));
  }

  wake_notice_katcp(d, r->r_statemachine_notice, NULL);
#endif
  return 0;
}

int roach_ping_mod(struct katcp_dispatch *d, struct katcp_notice *n, void *data)
{
#if 0
  struct kcs_sm *m;
  struct kcs_sm_state *s;
  struct katcp_stack *stack;
  struct katcp_stack_obj *a;
  struct katcp_type *type;
  struct katcp_roach *r;

  struct katcp_notice *pn;
  struct katcl_parse *p;
  struct katcp_job *j;
  
  s = data;
  if (s == NULL)
    return -1;

  m = s->s_sm;
  if (m == NULL)
    return -1;

  stack = m->m_stack;
  if (stack == NULL)
    return -1;

  a = pop_stack_katcp(stack);
  if (a == NULL){
    destroy_obj_stack_katcp(a);
    return -1;
  }

  type = find_name_type_katcp(d, KATCP_TYPE_ROACH);
  if (a->o_type == NULL || type == NULL || a->o_type != type){
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "roach ping mod: type mismatch");
    push_stack_obj_katcp(stack, a);
    return -1;
  }
  
  r = a->o_data;
  if (r == NULL){
    push_stack_obj_katcp(stack, a);
    return -1;
  }

  j = r->r_job; 
  r->r_statemachine_notice = n;

  p = create_parse_katcl();
  if (p == NULL){
    push_stack_obj_katcp(stack, a);
    return -1;
  }

  if (add_string_parse_katcl(p, KATCP_FLAG_FIRST | KATCP_FLAG_LAST | KATCP_FLAG_STRING, "?watchdog") < 0){
    push_stack_obj_katcp(stack, a);
    destroy_parse_katcl(p);
    return -1;
  }

  pn = register_parse_notice_katcp(d, NULL, p, &roach_ping_returns_mod, r);
  if (pn == NULL){
    push_stack_obj_katcp(stack, a);
    destroy_parse_katcl(p);
    return -1;
  }
  
  if (notice_to_job_katcp(d, j, pn) < 0){
    push_stack_obj_katcp(stack, a);
    destroy_parse_katcl(p);
    return -1;
  }
  
  log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "sent ping to %s", r->r_url->u_str);

  destroy_obj_stack_katcp(a);
  
  gettimeofday(&r->r_seen, NULL);
#endif
  return 0;
}

struct kcs_sm_edge *roach_ping_setup_mod(struct katcp_dispatch *d, struct kcs_sm_state *s)
{
  struct kcs_sm_edge *e;

  e = create_sm_edge_kcs(s, &roach_ping_mod);
  if (e == NULL)
    return NULL;

  return e;
}

int init_mod(struct katcp_dispatch *d)
{
  int rtn;
  
  if (check_code_version_katcp(d) != 0){
#ifdef DEBUG
    fprintf(stderr, "mod: ERROR was build against an incompatible katcp lib\n");
#endif
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "cannot load module katcp version mismatch");
    return -1;
  }

  log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "successfully loaded mod_config_parser");
 
#if 0
  rtn  = register_name_type_katcp(d, KATCP_TYPE_ROACH, KATCP_DEP_BASE, &print_roach_type_mod, &destroy_roach_type_mod, NULL, NULL, &parse_roach_type_mod);
#endif
  rtn  = register_name_type_katcp(d, KATCP_TYPE_URL, KATCP_DEP_BASE, &print_katcp_url_type_mod, &destroy_katcp_url_type_mod, NULL, NULL, &parse_katcp_url_type_mod);
  
  log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "added type:");
#if 0
  log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "%s", KATCP_TYPE_ROACH);
#endif
  log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "%s", KATCP_TYPE_URL);

  log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "added operations:");

  rtn += store_data_type_katcp(d, KATCP_TYPE_OPERATION, KATCP_DEP_BASE, KATCP_OPERATION_ROACH_CONNECT, &roach_connect_setup_mod, NULL, NULL, NULL, NULL, NULL);
  log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "%s", KATCP_OPERATION_ROACH_CONNECT);

  log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "added edges:");

  rtn += store_data_type_katcp(d, KATCP_TYPE_EDGE, KATCP_DEP_BASE, KATCP_EDGE_ROACH_PING, &roach_ping_setup_mod, NULL, NULL, NULL, NULL, NULL);
  log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "%s", KATCP_EDGE_ROACH_PING);

  log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "to see the full operation list: ?sm oplist");

  return rtn;
}
