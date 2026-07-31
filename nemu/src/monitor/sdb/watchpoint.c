/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "sdb.h"
#include <utils.h>

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char expr[128];
  word_t old_value;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

WP* new_wp() {
  assert(free_  != NULL);

  WP *wp = free_;
  free_ = free_ -> next;
  wp -> next = head;
  head = wp;

  return wp;
}

void free_wp(WP *wp) {
  WP *find = NULL;
  assert(wp != NULL);

  if(head == wp) {
    head = head -> next;
  } else {
    find = head;
    while (find != NULL && find -> next != wp) {
      find = find -> next;
    }
    find -> next = wp -> next;
  }
  wp -> next = free_;
  free_ = wp;
}

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

void init_wp(char *args, word_t val) {
  WP *wp = new_wp();

  strcpy(wp -> expr, args);
  wp -> old_value = val;

  printf("New watchpoint: expr = %s, old_value = 0x%x\n", wp -> expr, wp -> old_value);
}

void watchpoint_check() {
  WP *p = head;
  word_t new_value;
  while(p != NULL) {
    bool success = true;
    new_value = expr(p->expr, &success);
    if(!success) {
      printf("Print expression evaluation failed\n");
    }
    if(success && new_value != p -> old_value) {
      printf("Watchpoint N: NO = %d, expr = %s, old_value = 0x%x, new_value = 0x%x\n", p -> NO, p -> expr, p -> old_value, new_value);
      p->old_value = new_value;
      nemu_state.state = NEMU_STOP; 
    }
    p = p ->next;
  } 
}

/* TODO: Implement the functionality of watchpoint */
void watchpoint_display(void){
  if(head == NULL) {
    printf("Watchponit NULL\n");
  } else {
    WP *p = head;
    while(p != NULL) {
        printf("Watchpoint N: NO = %d, expr = %s, value = 0x%x\n", p -> NO, p -> expr, p -> old_value);
        p = p -> next;
    }
  }
}
