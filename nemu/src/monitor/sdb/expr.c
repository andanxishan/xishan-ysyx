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

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_NEQ, TK_AND, TK_OR, TK_NUM, TK_HEX, TK_REG
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

   //Have done (same):NOTYPE,(NUM,HEX,REG),(EQ,NEQ,OR),(all)
  {" +", TK_NOTYPE},    // spaces
  {"0[xX][0-9a-fA-F]+", TK_HEX},
  {"[0-9]+", TK_NUM},
  {"==", TK_EQ},        // equal
  {"!=", TK_NEQ},
  {"&&", TK_AND},
  {"\\|\\|", TK_OR},
  {"\\+", '+'},         // plus
  {"\\-", '-'},
  {"\\*", '*'},
  {"\\/", '/'},
  {"\\(", '('},
  {"\\)", ')'},
  {"\\$[a-zA-Z0-9]+", TK_REG},

};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;

          case TK_NUM:
            tokens[nr_token].type = TK_NUM;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
            break;
          
          case TK_HEX:
            tokens[nr_token].type = TK_HEX;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
            break;

          case TK_REG:
            tokens[nr_token].type = TK_REG;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
            break;

          case TK_EQ:
            tokens[nr_token].type = TK_EQ;
            nr_token++;
            break;

          case TK_NEQ:
            tokens[nr_token].type = TK_NEQ;
            nr_token++;
            break;

          case TK_AND:
            tokens[nr_token].type = TK_AND;
            nr_token++;
            break;

          case TK_OR:
            tokens[nr_token].type = TK_OR;
            nr_token++;
            break;
          
          case '+':
          case '-':
          case '*':
          case '/':
          case '(':
          case ')':
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;

          default: TODO();
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static bool check_parentheses(int p, int q) {
  int i;
  int depth = 0;

  if (tokens[p].type == '(' && tokens[q].type == ')') {
    for (i = p; i < q + 1; i ++) {
      if (tokens[i].type == '(') {
        depth++;
      } else if (tokens[i].type == ')') {
        depth--;
      }
      if (depth == 0) {
        if (i == q){
          return true;
        } else {
          return false;
        }
      }
    }
  } 

  return false;
}

static int find_main_op(int p,int q) {
  int i;
  int depth;

  depth = 0;
  for (i = q; i > p - 1; i --) {

    if (tokens[i].type == '(') {
      depth--;
    } 
    else if (tokens[i].type == ')') {
      depth++;
    }
    if (tokens[i].type == '+' || tokens[i].type == '-') {
        if (depth == 0) {
          return i;
        }
    }
  }

  depth = 0;
  for (i = q; i > p - 1; i --) {
 
    if (tokens[i].type == '(') {
      depth--;
    } 
    else if (tokens[i].type == ')') {
      depth++;
    }
    if (tokens[i].type == '*' || tokens[i].type == '/'){
      if (depth == 0) {
        return i;
      }
    }
  }
  assert(0);
}

word_t eval(int p, int q) {
  int op;
  int val1, val2;
  char op_type;

  if (p > q) {
    /* Bad expression */
    assert(0);
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    if (tokens[p].type == TK_NUM) {
      return atoi(tokens[p].str);
    } else if (tokens[p].type == TK_HEX) {
      return strtoul(tokens[p].str, NULL, 16);
    } else {
      assert(0);
    }
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  else {
    //op = the position of 主运算符 in the token expression;
    op = find_main_op(p, q);
    op_type = tokens[op].type;
    val1 = eval(p, op - 1);
    val2 = eval(op + 1, q);

    switch (op_type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': return val1 / val2;
      default: assert(0);
    }
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  *success = true;
  return eval(0, nr_token - 1);
  
}
