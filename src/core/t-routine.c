/***********************************************************************
**
**  REBOL [R3] Language Interpreter and Run-time Environment
**
**  Copyright 2014 Atronix Engineering, Inc.
**  REBOL is a trademark of REBOL Technologies
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**  http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
**
************************************************************************
**
**  Module:  t-routine.c
**  Summary: External Routine Support
**  Section: datatypes
**  Author:  Shixin Zeng
**  Notes:
**
***********************************************************************/

#include <stdio.h>
#include "sys-core.h"

#include <ffi.h>

#define QUEUE_EXTRA_MEM(v, p) do {\
	*(void**) SERIES_SKIP(v->extra_mem, SERIES_TAIL(v->extra_mem)) = p;\
	EXPAND_SERIES_TAIL(v->extra_mem, 1);\
} while (0)

static ffi_type * struct_type_to_ffi [STRUCT_TYPE_MAX];

static void init_type_map()
{
	if (struct_type_to_ffi[0]) return;
	struct_type_to_ffi[STRUCT_TYPE_UINT8] = &ffi_type_uint8;
	struct_type_to_ffi[STRUCT_TYPE_INT8] = &ffi_type_sint8;
	struct_type_to_ffi[STRUCT_TYPE_UINT16] = &ffi_type_uint16;
	struct_type_to_ffi[STRUCT_TYPE_INT16] = &ffi_type_sint16;
	struct_type_to_ffi[STRUCT_TYPE_UINT32] = &ffi_type_uint32;
	struct_type_to_ffi[STRUCT_TYPE_INT32] = &ffi_type_sint32;
	struct_type_to_ffi[STRUCT_TYPE_UINT64] = &ffi_type_uint64;
	struct_type_to_ffi[STRUCT_TYPE_INT64] = &ffi_type_sint64;

	struct_type_to_ffi[STRUCT_TYPE_FLOAT] = &ffi_type_float;
	struct_type_to_ffi[STRUCT_TYPE_DOUBLE] = &ffi_type_double;

	struct_type_to_ffi[STRUCT_TYPE_POINTER] = &ffi_type_pointer;
}

/***********************************************************************
**
*/	REBINT CT_Routine(REBVAL *a, REBVAL *b, REBINT mode)
/*
***********************************************************************/
{
	RL_Print("%s, %d\n", __func__, __LINE__);
	return -1;
}

static REBCNT n_struct_fields (REBSER *fields)
{
	REBCNT n_fields = 0;
	REBCNT i = 0;
	for (i = 0; i < SERIES_TAIL(fields); i ++) {
		struct Struct_Field *field = (struct Struct_Field*)SERIES_SKIP(fields, i);
		if (field->type != STRUCT_TYPE_STRUCT) {
			n_fields += field->dimension;
		} else {
			n_fields += n_struct_fields(field->fields);
		}
	}
	return n_fields;
}

static ffi_type* struct_to_ffi(REBVAL *out, REBSER *fields)
{
	ffi_type *args = (ffi_type*) SERIES_DATA(VAL_ROUTINE_FFI_ARGS(out));
	REBCNT i = 0, j = 0;
	REBCNT n_basic_type = 0;

	ffi_type *stype = OS_MAKE(sizeof(ffi_type));
	printf("allocated stype at: %p\n", stype);
	QUEUE_EXTRA_MEM(VAL_ROUTINE_INFO(out), stype);

	stype->size = stype->alignment = 0;
	stype->type = FFI_TYPE_STRUCT;

	stype->elements = OS_MAKE(sizeof(ffi_type *) * (1 + n_struct_fields(fields))); /* one extra for NULL */
	printf("allocated stype elements at: %p\n", stype->elements);
	QUEUE_EXTRA_MEM(VAL_ROUTINE_INFO(out), stype->elements);

	for (i = 0; i < SERIES_TAIL(fields); i ++) {
		struct Struct_Field *field = (struct Struct_Field*)SERIES_SKIP(fields, i);
		if (field->type != STRUCT_TYPE_STRUCT) {
			if (struct_type_to_ffi[field->type]) {
				REBINT n = 0;
				for (n = 0; n < field->dimension; n ++) {
					stype->elements[j++] = struct_type_to_ffi[field->type];
				}
			} else {
				return NULL;
			}
		} else {
			ffi_type *subtype = struct_to_ffi(out, field->fields);
			if (subtype) {
				REBINT n = 0;
				for (n = 0; n < field->dimension; n ++) {
					stype->elements[j++] = subtype;
				}
			} else {
				return NULL;
			}
		}
	}
	stype->elements[j] = NULL;

	return stype;
}

/* convert the type of "elem", and store it in "out" with index of "idx"
 */
static REBOOL rebol_type_to_ffi(REBVAL *out, REBVAL *elem, REBCNT idx)
{
	ffi_type **args = (ffi_type**) SERIES_DATA(VAL_ROUTINE_FFI_ARGS(out));
	REBVAL *rebol_args = (REBVAL*)SERIES_DATA(VAL_ROUTINE_ARGS(out));
	if (IS_WORD(elem)) {
		switch (VAL_WORD_CANON(elem)) {
			case SYM_UINT8:
				args[idx] = &ffi_type_uint8;
				if (idx) TYPE_SET(&rebol_args[idx], REB_INTEGER);
				break;
			case SYM_INT8:
				args[idx] = &ffi_type_sint8;
				if (idx) TYPE_SET(&rebol_args[idx], REB_INTEGER);
				break;
			case SYM_UINT16:
				args[idx] = &ffi_type_uint16;
				if (idx) TYPE_SET(&rebol_args[idx], REB_INTEGER);
				break;
			case SYM_INT16:
				args[idx] = &ffi_type_sint16;
				if (idx) TYPE_SET(&rebol_args[idx], REB_INTEGER);
				break;
			case SYM_UINT32:
				args[idx] = &ffi_type_uint32;
				if (idx) TYPE_SET(&rebol_args[idx], REB_INTEGER);
				break;
			case SYM_INT32:
				args[idx] = &ffi_type_sint32;
				if (idx) TYPE_SET(&rebol_args[idx], REB_INTEGER);
				break;
			case SYM_UINT64:
				args[idx] = &ffi_type_uint64;
				if (idx) TYPE_SET(&rebol_args[idx], REB_INTEGER);
				break;
			case SYM_INT64:
				args[idx] = &ffi_type_sint64;
				if (idx) TYPE_SET(&rebol_args[idx], REB_INTEGER);
				break;
			case SYM_FLOAT:
				args[idx] = &ffi_type_float;
				if (idx) TYPE_SET(&rebol_args[idx], REB_DECIMAL);
				break;
			case SYM_DOUBLE:
				args[idx] = &ffi_type_double;
				if (idx) TYPE_SET(&rebol_args[idx], REB_DECIMAL);
				break;
			case SYM_POINTER:
				args[idx] = &ffi_type_pointer;
				if (idx) {
					TYPE_SET(&rebol_args[idx], REB_INTEGER);
					TYPE_SET(&rebol_args[idx], REB_STRING);
					TYPE_SET(&rebol_args[idx], REB_BINARY);
					TYPE_SET(&rebol_args[idx], REB_VECTOR);
				}
				break;
			default:
				return FALSE;
		}
	} else if (IS_STRUCT(elem)) {
		ffi_type *ftype = struct_to_ffi(out, VAL_STRUCT_FIELDS(elem));
		if (ftype) {
			args[idx] = ftype;
			if (idx) {
				TYPE_SET(&rebol_args[idx], REB_STRUCT);
			}
		} else {
			return FALSE;
		}
	} else {
		return FALSE;
	}
	return TRUE;
}

/* make a copy of the argument 
 * arg referes to return value when idx = 0
 * function args start from idx = 1
 *
 * For FFI_TYPE_POINTER, a temperary pointer could be needed 
 * (whose address is returned). The pointer is stored in rebol 
 * stack, so DS_POP is needed after the function call is done.
 * The number to pop is returned by pop
 * */
static void *arg_to_ffi(REBRIN *rin, REBVAL *arg, REBCNT idx, REBINT *pop)
{
	ffi_type **args = (ffi_type**)SERIES_DATA(rin->args);
	switch (args[idx]->type) {
		case FFI_TYPE_UINT8:
			if (!IS_INTEGER(arg)) {
				Trap_Arg(arg);
			} else {
#ifdef BIG_ENDIAN
				u8 i = (u8) VAL_INT64(arg);
				memcpy(&VAL_INT64(arg), &i, sizeof(u8));
#endif
				return &VAL_INT64(arg);
			}
		case FFI_TYPE_SINT8:
			if (!IS_INTEGER(arg)) {
				Trap_Arg(arg);
			} else {
#ifdef BIG_ENDIAN
				i8 i = (i8) VAL_INT64(arg);
				memcpy(&VAL_INT64(arg), &i, sizeof(i8));
#endif
				return &VAL_INT64(arg);
			}
		case FFI_TYPE_UINT16:
			if (!IS_INTEGER(arg)) {
				Trap_Arg(arg);
			} else {
#ifdef BIG_ENDIAN
				u16 i = (u16) VAL_INT64(arg);
				memcpy(&VAL_INT64(arg), &i, sizeof(u16));
#endif
				return &VAL_INT64(arg);
			}
		case FFI_TYPE_SINT16:
			if (!IS_INTEGER(arg)) {
				Trap_Arg(arg);
			} else {
#ifdef BIG_ENDIAN
				i16 i = (i16) VAL_INT64(arg);
				memcpy(&VAL_INT64(arg), &i, sizeof(i16));
#endif
				return &VAL_INT64(arg);
			}
		case FFI_TYPE_UINT32:
			if (!IS_INTEGER(arg)) {
				Trap_Arg(arg);
			} else {
#ifdef BIG_ENDIAN
				u32 i = (u32) VAL_INT64(arg);
				memcpy(&VAL_INT64(arg), &i, sizeof(u32));
#endif
				return &VAL_INT64(arg);
			}
		case FFI_TYPE_SINT32:
			if (!IS_INTEGER(arg)) {
				Trap_Arg(arg);
			} else {
#ifdef BIG_ENDIAN
				i32 i = (i32) VAL_INT64(arg);
				memcpy(&VAL_INT64(arg), &i, sizeof(i32));
#endif
				return &VAL_INT64(arg);
			}
		case FFI_TYPE_UINT64:
		case FFI_TYPE_SINT64:
			if (!IS_INTEGER(arg)) {
				Trap_Arg(arg);
			}
			return &VAL_INT64(arg);
		case FFI_TYPE_POINTER:
			switch (VAL_TYPE(arg)) {
				case REB_INTEGER:
					return &VAL_INT64(arg);
				case REB_STRING:
				case REB_BINARY:
				case REB_VECTOR:
					{
						DS_PUSH_INTEGER((REBUPT)VAL_DATA(arg));
						(*pop) ++;
						return &VAL_INT64(DS_TOP);
					}
				defaut:
					Trap_Arg(arg);
			}
		case FFI_TYPE_FLOAT:
			/* hackish, store the signle precision floating point number in a double precision variable */
			if (!IS_DECIMAL(arg)) {
				Trap_Arg(arg);
			} else {
				float a = (float)VAL_DECIMAL(arg);
				memcpy(&VAL_DECIMAL(arg), &a, sizeof(a));
				return &VAL_DECIMAL(arg);
			}
		case FFI_TYPE_DOUBLE:
			if (!IS_DECIMAL(arg)) {
				Trap_Arg(arg);
			}
			return &VAL_DECIMAL(arg);
		case FFI_TYPE_STRUCT:
			/* make a copy of old binary data, such that the original one won't be modified */
			if (idx == 0) {/* returning a struct */
				Copy_Struct(&rin->rvalue, &VAL_STRUCT(arg));
			} else {
				if (IS_STRUCT(arg)) {
					VAL_STRUCT_DATA_BIN(arg) = Copy_Series(VAL_STRUCT_DATA_BIN(arg));
				} else {
					Trap_Arg(arg);
				}
			}
			return SERIES_SKIP(VAL_STRUCT_DATA_BIN(arg), VAL_STRUCT_OFFSET(arg));
		case FFI_TYPE_VOID:
			if (!idx) {
				return NULL;
			} else {
				Trap_Arg(arg);
			}
		default:
			Trap_Arg(arg);
	}
	return NULL;
}

static void prep_rvalue(REBRIN *rin,
						REBVAL *val)
{
	ffi_type * rtype = *(ffi_type**) SERIES_DATA(rin->args);
	switch (rtype->type) {
		case FFI_TYPE_UINT8:
		case FFI_TYPE_SINT8:
		case FFI_TYPE_UINT16:
		case FFI_TYPE_SINT16:
		case FFI_TYPE_UINT32:
		case FFI_TYPE_SINT32:
		case FFI_TYPE_UINT64:
		case FFI_TYPE_SINT64:
		case FFI_TYPE_POINTER:
			SET_INTEGER(val, 0);
			break;
		case FFI_TYPE_FLOAT:
		case FFI_TYPE_DOUBLE:
			SET_DECIMAL(val, 0);
			break;
		case FFI_TYPE_STRUCT:
			SET_TYPE(val, REB_STRUCT);
			break;
		case FFI_TYPE_VOID:
			break;
		default:
			Trap_Arg(val);
	}
}

/* convert the return value to rebol
 */
static void ffi_to_rebol(REBRIN *rin,
						 ffi_type *ffi_rtype,
						 void *ffi_rvalue,
						 REBVAL *rebol_ret)
{
	switch (ffi_rtype->type) {
		case FFI_TYPE_UINT8:
			SET_INTEGER(rebol_ret, *(u8*)ffi_rvalue);
			break;
		case FFI_TYPE_SINT8:
			SET_INTEGER(rebol_ret, *(i8*)ffi_rvalue);
			break;
		case FFI_TYPE_UINT16:
			SET_INTEGER(rebol_ret, *(u16*)ffi_rvalue);
			break;
		case FFI_TYPE_SINT16:
			SET_INTEGER(rebol_ret, *(i16*)ffi_rvalue);
			break;
		case FFI_TYPE_UINT32:
			SET_INTEGER(rebol_ret, *(u32*)ffi_rvalue);
			break;
		case FFI_TYPE_SINT32:
			SET_INTEGER(rebol_ret, *(i32*)ffi_rvalue);
			break;
		case FFI_TYPE_UINT64:
			SET_INTEGER(rebol_ret, *(u64*)ffi_rvalue);
			break;
		case FFI_TYPE_SINT64:
			SET_INTEGER(rebol_ret, *(i64*)ffi_rvalue);
			break;
		case FFI_TYPE_POINTER:
			SET_INTEGER(rebol_ret, (REBUPT)*(void**)ffi_rvalue);
			break;
		case FFI_TYPE_FLOAT:
			SET_DECIMAL(rebol_ret, *(float*)ffi_rvalue);
			break;
		case FFI_TYPE_DOUBLE:
			SET_DECIMAL(rebol_ret, *(double*)ffi_rvalue);
			break;
		case FFI_TYPE_STRUCT:
			break;
		case FFI_TYPE_VOID:
			break;
		default:
			Trap_Arg(rebol_ret);
	}
}

/***********************************************************************
**
*/	void Call_Routine(REBVAL *rot, REBSER *args, REBVAL *ret)
/*
***********************************************************************/
{
	REBCNT i = 0;
	void *rvalue = NULL;
	REBSER *ser = Make_Series(SERIES_TAIL(VAL_ROUTINE_FFI_ARGS(rot)) - 1, sizeof(void *), FALSE);
	void ** ffi_args = (void **) SERIES_DATA(ser);
	REBINT pop = 1; /* for tmp */
	REBVAL *tmp = NULL;

	/* save ser on stack such that it won't be GC'ed */
	DS_PUSH_NONE;
	tmp = DS_TOP;
	SET_TYPE(tmp, REB_BLOCK);
	VAL_SERIES(tmp) = ser;

	for (i = 1; i < SERIES_TAIL(VAL_ROUTINE_FFI_ARGS(rot)); i ++) {
		ffi_args[i - 1] = arg_to_ffi(VAL_ROUTINE_INFO(rot), BLK_SKIP(args, i - 1), i, &pop);
	}
	prep_rvalue(VAL_ROUTINE_INFO(rot), ret);
	rvalue = arg_to_ffi(VAL_ROUTINE_INFO(rot), ret, 0, &pop);
	ffi_call(VAL_ROUTINE_CIF(rot),
			 (void (*) (void))VAL_ROUTINE_FUNCPTR(rot),
			 rvalue,
			 ffi_args);
	ffi_to_rebol(VAL_ROUTINE_INFO(rot), ((ffi_type**)SERIES_DATA(VAL_ROUTINE_FFI_ARGS(rot)))[0], rvalue, ret);
	DSP -= pop;
}

/***********************************************************************
**
*/	void Free_Routine(REBRIN *rin)
/*
***********************************************************************/
{
	REBCNT n = 0;
	for (n = 0; n < SERIES_TAIL(rin->extra_mem); n ++) {
		void *addr = *(void **)SERIES_SKIP(rin->extra_mem, n);
		printf("freeing %p\n", addr);
		OS_FREE(addr);
	}

	UNMARK_ROUTINE(rin);
	Free_Node(RIN_POOL, (REBNOD*)rin);
}

static void process_type_block(REBVAL *out, REBVAL *blk, REBCNT n)
{
	if (IS_BLOCK(blk)) {
		REBVAL *t = VAL_BLK_DATA(blk);
		if (IS_WORD(t)
			&& VAL_WORD_CANON(t) == SYM_STRUCT_TYPE) {
			/* followed by struct definition */
			++ t;
			if (!IS_BLOCK(t) || VAL_LEN(blk) != 2) {
				Trap_Arg(blk);
			}
			DS_PUSH_NONE;
			if (!MT_Struct(DS_TOP, t, REB_STRUCT)) {
				Trap_Arg(blk);
			}
			if (!rebol_type_to_ffi(out, DS_TOP, n)) {
				Trap_Arg(blk);
			}
			if (n == 0) { /* return type */
				Copy_Struct(&VAL_STRUCT(DS_TOP), &VAL_ROUTINE_RVALUE(out));
			}

			DS_POP;
		} else {
			if (VAL_LEN(blk) != 1) {
				Trap_Arg(blk);
			}
			if (!rebol_type_to_ffi(out, t, n)) {
				Trap_Arg(t);
			}
			if (n == 0) { /* return type*/
				ffi_type **args = (ffi_type**) SERIES_DATA(VAL_ROUTINE_FFI_ARGS(out));
				if (args[0]->type == FFI_TYPE_STRUCT) {
					Copy_Struct(&VAL_STRUCT(t), &VAL_ROUTINE_RVALUE(out));
				}
			}
		}
	} else {
		Trap_Arg(blk);
	}
}

/***********************************************************************
**
*/	REBFLG MT_Routine(REBVAL *out, REBVAL *data, REBCNT type)
/*
** format:
** make routine! [[
** 	"document"
** 	arg1 [type1 type2] "note"
** 	arg2 [type3] "note"
** 	...
** 	argn [typen] "note"
** 	return: [type]
** 	abi: word
** ] lib "name"]
**
***********************************************************************/
{
	RL_Print("%s, %d\n", __func__, __LINE__);
	ffi_type ** args = NULL;
	REBVAL *blk = NULL;
	REBCNT eval_idx = 0; /* for spec block evaluation */
	REBSER *extra_mem = NULL;
	REBFLG ret = TRUE;
	void (*func) (void) = NULL;

	if (!IS_BLOCK(data)) {
		return FALSE;
	}

	SET_TYPE(out, REB_ROUTINE);

	VAL_ROUTINE_INFO(out) = Make_Node(RIN_POOL);
	USE_ROUTINE(VAL_ROUTINE_INFO(out));

#define N_ARGS 8

	VAL_ROUTINE_SPEC(out) = Copy_Series(VAL_SERIES(data));
	VAL_ROUTINE_FFI_ARGS(out) = Make_Series(N_ARGS, sizeof(ffi_type*), FALSE);
	VAL_ROUTINE_ARGS(out) = Make_Block(N_ARGS);
	Append_Value(VAL_ROUTINE_ARGS(out)); //first word should be 'self', but ignored here.

	VAL_ROUTINE_ABI(out) = FFI_DEFAULT_ABI;
	VAL_ROUTINE_LIB(out) = NULL;

	CLEAR(&VAL_ROUTINE_RVALUE(out), sizeof(REBSTU));

	extra_mem = Make_Series(N_ARGS, sizeof(void*), FALSE);
	VAL_ROUTINE_EXTRA_MEM(out) = extra_mem;

	args = (ffi_type**)SERIES_DATA(VAL_ROUTINE_FFI_ARGS(out));
	EXPAND_SERIES_TAIL(VAL_ROUTINE_FFI_ARGS(out), 1); //reserved for return type
	args[0] = &ffi_type_void; //default return type

	init_type_map();

	blk = VAL_BLK_DATA(data);
	if (VAL_LEN(data) != 3
		|| !IS_BLOCK(&blk[0])
		|| !IS_LIBRARY(&blk[1])
		|| !IS_STRING(&blk[2])) {
		Trap_Arg(data);
	}

	VAL_ROUTINE_LIB(out) = VAL_LIB_HANDLE(&blk[1]);
	if (!VAL_ROUTINE_LIB(out)) {
		RL_Print("lib is not open\n");
		ret = FALSE;
	}
	TERM_SERIES(VAL_SERIES(&blk[2]));
	func = OS_FIND_FUNCTION(LIB_FD(VAL_ROUTINE_LIB(out)), VAL_DATA(&blk[2]));
	if (!func) {
		RL_Print("Couldn't find function\n");
		ret = FALSE;
	} else {
		VAL_ROUTINE_FUNCPTR(out) = func;
	}

	blk = VAL_BLK_DATA(&blk[0]);
	REBCNT n = 1; /* arguments start with the index 1 (return type has a index of 0) */
	REBCNT has_return = 0;
	REBCNT has_abi = 0;
	if (NOT_END(blk) && IS_STRING(blk)) {
		++ blk;
	}
	while (NOT_END(blk)) {
		switch (VAL_TYPE(blk)) {
			case REB_WORD:
				{
					REBVAL *v = Append_Value(VAL_ROUTINE_ARGS(out));
					Init_Word(v, VAL_WORD_SYM(blk));
					EXPAND_SERIES_TAIL(VAL_ROUTINE_FFI_ARGS(out), 1);

					++ blk;
					process_type_block(out, blk, n);
					++ blk;
					if (IS_STRING(blk)) { /* argument notes, ignoring */
						++ blk;
					}
				}
				n ++;
				break;
			case REB_SET_WORD:
				switch (VAL_WORD_CANON(blk)) {
					case SYM_ABI:
						++ blk;
						if (!IS_WORD(blk) || has_abi > 1) {
							Trap_Arg(blk);
						}
						switch (VAL_WORD_CANON(blk)) {
							case SYM_DEFAULT:
								VAL_ROUTINE_ABI(out) = FFI_DEFAULT_ABI;
								break;
							case SYM_STDCALL:
								VAL_ROUTINE_ABI(out) = FFI_STDCALL;
								break;
							case SYM_UNIX64:
								VAL_ROUTINE_ABI(out) = FFI_UNIX64;
								break;
#ifdef X86_WIN64
							case SYM_WIN64:
								VAL_ROUTINE_ABI(out) = FFI_WIN64;
								break;
#endif
#ifdef X86_WIN32
							case SYM_MS_CDECL:
								VAL_ROUTINE_ABI(out) = FFI_MS_CDECL;
								break;
#endif
							case SYM_SYSV:
								VAL_ROUTINE_ABI(out) = FFI_SYSV;
								break;
							case SYM_THISCALL:
								VAL_ROUTINE_ABI(out) = FFI_THISCALL;
								break;
							case SYM_FASTCALL:
								VAL_ROUTINE_ABI(out) = FFI_FASTCALL;
								break;
							default:
								Trap_Arg(blk);
						}
						has_abi ++;
						break;
					case SYM_RETURN:
						if (has_return > 1) {
							Trap_Arg(blk);
						}
						has_return ++;
						++ blk;
						process_type_block(out, blk, 0);
						break;
					default:
						Trap_Arg(blk);
				}
				++ blk;
				break;
			default:
				Trap_Arg(blk);
		}
	}

	VAL_ROUTINE_CIF(out) = OS_MAKE(sizeof(ffi_cif));
	printf("allocated cif at: %p\n", VAL_ROUTINE_CIF(out));
	QUEUE_EXTRA_MEM(VAL_ROUTINE_INFO(out), VAL_ROUTINE_CIF(out));

	if (FFI_OK != ffi_prep_cif((ffi_cif*)VAL_ROUTINE_CIF(out),
							   VAL_ROUTINE_ABI(out),
							   SERIES_TAIL(VAL_ROUTINE_FFI_ARGS(out)) - 1,
							   args[0],
							   &args[1])) {
		RL_Print("Couldn't prep CIF\n");
		ret = FALSE;
	}

	RL_Print("%s, %d, ret = %d\n", __func__, __LINE__, ret);
	return ret;
}

/***********************************************************************
**
*/	REBTYPE(Routine)
/*
***********************************************************************/
{
	REBVAL *val;
	REBVAL *arg;
	REBSTU *strut;
	REBSTU *nstrut;
	REBCNT index;
	REBCNT tail;
	REBCNT len;

	arg = D_ARG(2);
	val = D_ARG(1);
	strut = 0;

	REBVAL *ret = DS_RETURN;
	// unary actions
	switch(action) {
		case A_MAKE:
			RL_Print("%s, %d, Make routine action\n", __func__, __LINE__);
		case A_TO:
			if (IS_ROUTINE(val)) {
				Trap_Types(RE_EXPECT_VAL, REB_ROUTINE, VAL_TYPE(arg));
			} else if (!IS_BLOCK(arg) || !MT_Routine(ret, arg, REB_ROUTINE)) {
				Trap_Types(RE_EXPECT_VAL, REB_BLOCK, VAL_TYPE(arg));
			}
			break;
		case A_REFLECT:
			{
				REBINT n = VAL_WORD_CANON(arg); // zero on error
				switch (n) {
					case SYM_SPEC:
						Set_Block(ret, Clone_Block(VAL_ROUTINE_SPEC(val)));
						Unbind_Block(VAL_BLK(val), TRUE);
						break;
					case SYM_ADDR:
						SET_INTEGER(ret, (REBUPT)VAL_ROUTINE_FUNCPTR(val));
						break;
					default:
						Trap_Reflect(REB_STRUCT, arg);
				}
			}
			break;
		default:
			Trap_Action(REB_ROUTINE, action);
	}
	return R_RET;
}