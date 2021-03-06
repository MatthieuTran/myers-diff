#include "myers.h"
#include "options.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

myers *myers_make(block *block1, block *block2) {
  myers *m = malloc(sizeof(myers));
  m->block1 = block1;
  m->block2 = block2;
  m->num_edits = 0;

  return m;
}

void myers_do(myers *m) {
  int **moveset; // Moveset
  edit **edits;  // Edit collection

  // printf("MAKING MOVESET\n");

  /* Generate the moveset for the blocks */
  moveset = myers_make_moveset(m);

  // printf("BACKTRACKING NOW\n");

  /* Backtrack to look for best path */
  edits = myers_backtrack(m, moveset);

  // printf("PRINTING RESULTS\n");

  if (m->num_edits > 0) {
    filesequal = 0;
  }

  /* Print results */
  myers_print(m, edits);
}

void myers_clone_path(int **moveset, int *path, int size, int length) {
  int *cloned_path = malloc(sizeof(int) * size);
  moveset[length] = cloned_path;

  for (size_t i = 0; i < size; i++) {
    cloned_path[i] = path[i];
  }
}

int **myers_make_moveset(myers *m) {
  int **moveset, *path;
  size_t max_tries = m->block1->line_count +
                     m->block2->line_count; // Maximum possible # of moves
  size_t max_index = 2 * max_tries + 1, moves_length = 0, prev, next;
  int x, y;
  int left = 0;

  path = malloc(sizeof(int) * (max_index));
  moveset = malloc(sizeof(path) * max_tries);

  /* Initialize all values to -1 except first index */
  for (size_t i = 0; i < max_index; i++) {
    path[i] = i != 1 ? 0 : 0;
  }

  for (int d = 0; d < max_tries + 1; d++) { // Path
    myers_clone_path(moveset, path, max_index, moves_length++);

    for (int k = -d; k < d + 1; k += 2) { // Move
      prev = k - 1 >= 0 ? k - 1 : max_index + k - 1;
      next = k + 1 >= 0 ? k + 1 : max_index + k + 1;

      if (prev == max_index) {
        prev = 0;
      }

      if (next >= max_index) {
        next = next - max_index;
      }

      /* Determine to move upwards or downwards */
      if (k == -d || (k != d && path[prev] < path[next])) {
        x = path[next];
        left = 0;
      } else {
        x = path[prev] + 1;
        left = 1;
      }

      y = x - k;

      int (*cmpfnc)(const char *, const char *);
      cmpfnc = ignorecase ? stricmp : strcmp;
      int comp = 0;
      /* Check for diagonals and move accordingly */
      while (x < m->block1->line_count && y < m->block2->line_count &&
             (comp = cmpfnc(m->block1->lines[x]->content,
                            m->block2->lines[y]->content)) == 0) {
        ++x;
        ++y;
      }

      path[k >= 0 ? k : max_index + k] = x;

      if (x >= m->block1->line_count && y >= m->block2->line_count) {
        m->shortest_length = d;
        free(path);

        return moveset;
      }
    }
  }

  return NULL;
}

edit *myers_handle_op(myers *m, int prev_x, int prev_y, int x, int y) {
  edit *e;

  if (x == prev_x) {
    e = edit_make(OPERATION_INSERT, NULL, m->block2, -1, prev_y);
  } else if (y == prev_y) {
    e = edit_make(OPERATION_DELETE, m->block1, NULL, prev_x, -1);
  } else {
    e = edit_make(OPERATION_EQUALS, m->block1, m->block2, prev_x, prev_y);
  }

  return e;
}

edit **myers_backtrack(myers *m, int **moveset) {
  int x, y, k, prev_k, prev_x, prev_y, prev, next;
  edit **edits; // Collection of edits
  size_t max_tries = m->block1->line_count + m->block2->line_count;
  int max_index = 2 * max_tries + 1, k_index;

  x = m->block1->line_count;
  y = m->block2->line_count;

  edits = malloc(sizeof(edit) * m->shortest_length);

  for (int d = m->shortest_length; d > -1; d--) {
    k = x - y; // from y = x - k
    k_index = k < 0 ? max_index + k : k;
    // k_index = k;
    // printf("%d, %d -> %d", x, y, k_index);

    /* Determine what the previous k was */
    if (k == -d ||
        (k != d && moveset[d][k_index - 1] < moveset[d][k_index + 1])) {
      prev_k = k + 1;
    } else {
      prev_k = k - 1;
    }
    // printf("k=%d and prev_k=%d", k, prev_k);

    /* Calculate previous y */
    prev_x = moveset[d][prev_k < 0 ? max_index + prev_k : prev_k];
    prev_y = prev_x - prev_k;

    /* Check for diagonals */
    while (x > prev_x && y > prev_y) {
      edit *e = myers_handle_op(m, x - 1, y - 1, x, y);
      edits[m->num_edits++] = e;

      --x;
      --y;
    }

    /* Go to position before diagonals */
    if (d > 0) { // no previous pos to move back to, skip
      edit *e = myers_handle_op(m, prev_x, prev_y, x, y);
      edits[m->num_edits++] = e;
    }

    // printf("(%d, %d) => (%d, %d)\n", x, y, prev_x, prev_y);

    /* set x and y to values in prev round and continue */
    x = prev_x;
    y = prev_y;
  }

  return edits;
}

void myers_print(myers *m, edit **edits) {
  int prev = 4;
  char sign;
  if (show_unified) {
    printf("*** %s\n--- %s\n\n", filenames[0], filenames[1]);
  }

  for (int i = --m->num_edits; i > -1; i--) {
    // printf("%d", edits[i]->operation);
    if (show_unified || show_context) {
      if (edits[i]->operation == OPERATION_INSERT) {
        if (prev != RIGHT_FILE) {
          printf(
              "------------------------------------------------------------\n");
        }
        sign = '+';
        prev = RIGHT_FILE;
      } else if (diffnormal && edits[i]->operation == OPERATION_DELETE) {
        if (prev != LEFT_FILE) {
          printf(
              "************************************************************\n");
        }
        sign = '-';
        prev = LEFT_FILE;
      } else {
        sign = '=';
        prev = EQUAL_FILE;
      }

      printf("%c %d %d %s", sign, edits[i]->old_line + 1,
             edits[i]->new_line + 1, edits[i]->text(edits[i]));
    } else {
      if (edits[i]->operation == OPERATION_INSERT) {
        if (diffnormal && prev != OPERATION_INSERT && !show_brief) {
          printf("filler\n");
        }
        printright(edit_text(edits[i]));
        // sign = '+';
        prev = OPERATION_INSERT;
      } else if (diffnormal && edits[i]->operation == OPERATION_DELETE) {
        if (prev != OPERATION_DELETE && !show_brief) {
          printf("filler\n");
        }
        printleft(edit_text(edits[i]));
        // sign = '-';
        prev = OPERATION_DELETE;
      } else {
        if ((show_sidebyside || show_unified) && !suppress_common) {
          if (diffnormal && prev != OPERATION_EQUALS && !show_brief) {
            printf("filler\n");
          }

          printboth(edit_text(edits[i]));
          // sign = '=';
          prev = OPERATION_EQUALS;
        }
      }
    }
  }
}
