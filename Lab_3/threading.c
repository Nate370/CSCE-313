#include <threading.h>
#include <stdio.h>

void t_init()
{
        for (int i = 0; i < NUM_CTX; i++){
		contexts[i].state = INVALID;
	}
	contexts[0].state = VALID;
	getcontext(&(contexts[0].context));
	current_context_idx = 0;
}

int32_t t_create(fptr foo, int32_t arg1, int32_t arg2)
{
	uint8_t next_context_idx = current_context_idx;
	for (uint8_t i = 0; i < NUM_CTX; i++){
		if (contexts[i].state == INVALID){
			next_context_idx = i;
			break;
		}
	}
	if(current_context_idx == next_context_idx){
		return -1;
	}
	contexts[next_context_idx].state = VALID;
	getcontext(&(contexts[next_context_idx].context));
	contexts[next_context_idx].context.uc_stack.ss_sp = (char*)malloc(STK_SZ);
	contexts[next_context_idx].context.uc_stack.ss_size = STK_SZ;
	contexts[next_context_idx].context.uc_stack.ss_flags = 0;
	contexts[next_context_idx].context.uc_link = NULL;
	makecontext(&(contexts[next_context_idx].context), (void (*)()) foo, 2, arg1, arg2);

	return 0;
}

int32_t t_yield()
{
	ucontext_t* current_context_ptr = &(contexts[current_context_idx].context);
        getcontext(current_context_ptr);
	ucontext_t* next_context_ptr = NULL;
	uint8_t next_context_idx = current_context_idx;
	for (uint8_t i = 0; i < NUM_CTX; i++){
		if (contexts[i].state == VALID && i != current_context_idx){
			next_context_ptr = &(contexts[i].context);
			next_context_idx = i;
			break;
		}
	}
	if (current_context_idx == next_context_idx || next_context_ptr == NULL){
		return -1;
	}
	current_context_idx = next_context_idx;
	swapcontext(current_context_ptr, next_context_ptr);

	int32_t valid_contexts = 0;
	for (int i = 0; i < NUM_CTX; i++){
		if (contexts[i].state == VALID){
			valid_contexts++;
		}
	}
	return valid_contexts;
}

void t_finish()
{
        free(contexts[current_context_idx].context.uc_stack.ss_sp);
	contexts[current_context_idx].context.uc_stack.ss_size = 0;
	memset(&(contexts[current_context_idx].context), 0, sizeof(ucontext_t));
	contexts[current_context_idx].state = INVALID;
	swapcontext(&(contexts[current_context_idx].context), &(contexts[0].context));
}
