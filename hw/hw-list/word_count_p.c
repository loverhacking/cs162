/*
 * Implementation of the word_count interface using Pintos lists and pthreads.
 *
 * You may modify this file, and are expected to modify it.
 */

/*
 * Copyright © 2021 University of California, Berkeley
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 #ifndef PINTOS_LIST
 #error "PINTOS_LIST must be #define'd when compiling word_count_lp.c"
 #endif
 
 #ifndef PTHREADS
 #error "PTHREADS must be #define'd when compiling word_count_lp.c"
 #endif
 
 #include "word_count.h"
 
 void init_words(word_count_list_t* wclist) { 
   /* TODO */
   list_init(&wclist->lst);
   pthread_mutex_init(&wclist->lock, NULL);
 }
 
 size_t len_words(word_count_list_t* wclist) {
   /* TODO */
   return list_size(&wclist->lst);
 }
 
 word_count_t* find_word(word_count_list_t* wclist, char* word) {
   /* TODO */
   word_count_t* wc = NULL;
   pthread_mutex_lock(&wclist->lock);
   for (struct list_elem* e = list_begin(&wclist->lst); e != list_end(&wclist->lst); e = list_next(e)) {
     wc = list_entry(e, word_count_t, elem);
     if (strcmp(wc->word, word) == 0) {
       break;
     }
   }
   pthread_mutex_unlock(&wclist->lock);
   return wc;
 }
 
 word_count_t* add_word(word_count_list_t* wclist, char* word) {
   /* TODO */
 
   // Allocate memory for the new word count object before the critical section
   word_count_t* new_wc = (word_count_t*)malloc(sizeof(word_count_t));
   if (new_wc == NULL) return NULL;  
 
   new_wc->word = (char*)malloc(strlen(word) + 1);
   if (new_wc->word == NULL) {
     free(new_wc);
     return NULL;
   }
   strcpy(new_wc->word, word);
   new_wc->count = 1;
   
   // Enter the critical section
   pthread_mutex_lock(&wclist->lock);
 
   word_count_t* existing = NULL;
   for (struct list_elem* e = list_begin(&wclist->lst); e != list_end(&wclist->lst); e = list_next(e)) {
     word_count_t* wc = list_entry(e, word_count_t, elem);
     if (strcmp(wc->word, word) == 0) {
       existing = wc;
       break;
     }
   }
   if (existing != NULL) {
     // Word exists, increment its count
     existing->count++;
     pthread_mutex_unlock(&wclist->lock);
     // if the word exists
     // free the memory allocated for the new word count object
     free(new_wc->word);
     free(new_wc);
     return existing;
   }
   // if the word does not exist, add it to the list
   list_push_back(&wclist->lst, &new_wc->elem);
   pthread_mutex_unlock(&wclist->lock);
   return new_wc;
 }
 
 void fprint_words(word_count_list_t* wclist, FILE* outfile) {
   /* TODO */
   /* Please follow this format: fprintf(<file>, "%i\t%s\n", <count>, <word>); */
   for (struct list_elem* e = list_begin(&wclist->lst); e != list_end(&wclist->lst); e = list_next(e)) {
     word_count_t* wc = list_entry(e, word_count_t, elem);
     fprintf(outfile, "%i\t%s\n", wc->count, wc->word);
   }
 }
 
 static bool less_list(const struct list_elem* ewc1, const struct list_elem* ewc2, void* aux) {
   // Convert list_elem pointers to word_count_t pointers
   const word_count_t* wc1 = list_entry(ewc1, word_count_t, elem);
   const word_count_t* wc2 = list_entry(ewc2, word_count_t, elem);
   
   bool(*comp)(const word_count_t*, const word_count_t*) = aux;
   return comp(wc1, wc2);
 }
 
 void wordcount_sort(word_count_list_t* wclist,
                     bool less(const word_count_t*, const word_count_t*)) {
   /* TODO */
   pthread_mutex_lock(&wclist->lock);
   list_sort(&wclist->lst, less_list, less);
   pthread_mutex_unlock(&wclist->lock);
 }
 
