#include <kernel/json.h>
#include <stdio.h>

#define MAX_JSON_DEPTH 10

struct json_handler;

typedef	errno_t 	(*processor_func_t)	( struct json_handler *state, int level, jsmntype_t type, const char*value, size_t value_len );


typedef struct json_handler
{
	//errno_t 	(*processor)	( struct json_handler *state, int level, jsmntype_t type, const char*value, size_t value_len );
	processor_func_t	proc_func;
	void *				proc_state;

	const char *		string[MAX_JSON_DEPTH];
	size_t				len[MAX_JSON_DEPTH];

	errno_t				error; // if not 0 - we failed
} json_handler_t;

void jh_init( json_handler_t *jhp )
{
	jhp->error = 0;

	int i;
	for( i = 0; i < MAX_JSON_DEPTH; i++ )
	{
		jhp->string[i] = 0;
		jhp->len[i] = 0;
	}
}

//! Not equal
int json_strneq( const char *s1, size_t l1, const char *s2 )
{
	if( strlen(s2) != l1 ) return 1;
	return strncmp( s1, s2, l1);
}


json_handler_t jh;

#define CHECK_TYPE( __t ) \
	if( t->type != (__t) ) { \
		jhp->error = EINVAL; \
		printf("level %d type is %d , not %d", level, t->type, (__t) ); \
	}

#define CHECK_VALUE( __v ) \
	if( json_strneq( s, l, (__v)) ) { \
		jhp->error = EINVAL; \
		printf("level %d value is '%.*s' , not '%s'", level, l, s, (__v) ); \
	}

#define LEVEL_IS( __l, __s ) ( !json_strneq( jhp->string[__l], jhp->len[__l], (__s) ) )

void json_process_token(jsmntok_t * t, const char *full_string, int level )
{
	json_handler_t *jhp = &jh;

	if(level >= MAX_JSON_DEPTH)
	{
		printf("JSON too deep");
		return;
	}

	const char *s = full_string+t->start;
	size_t l = t->end-t->start;

	if( level == 0 )
	{
		// restart processing

		// TODO consume what we parsed before.
		// TODO consume last one elsewhere

		jh_init(jhp);

	}

	jhp->len[level] = l;
	jhp->string[level] = s;

	if(jhp->error) return;

	switch(level)
	{
		case 0:
			CHECK_TYPE(JSMN_STRING)
			CHECK_VALUE("object")
		break;

		case 1:
			CHECK_TYPE(JSMN_OBJECT)
		break;

		case 2: // will check on 3
/*		
			if( 
				json_strcmp( s, l, "class" ) &&
				json_strcmp( s, l, "value" )
			)
				jhp->error = EINVAL;
*/
			break;
		case 3:
			if( LEVEL_IS( 2, "class" ) )
			{
				printf("class %.*s\n", l, s );
			}
			else if( LEVEL_IS( 2, "value" ) )
			{
				printf("value %.*s\n", l, s );
			}
			else
			{
				printf("not class or value\n");
				jhp->error = EINVAL;
			}

	}

}


void json_dump_token(jsmntok_t * t, const char *full_string)
{
			jsmntok_t tt = *t;

			const char *type = "?";
			switch(tt.type)
			{
			case JSMN_UNDEFINED: type = "undef"; break;
			case JSMN_OBJECT:	 type = "object"; break;
			case JSMN_ARRAY:	 type = "array"; break;
			case JSMN_STRING: 	 type = "string"; break;
			case JSMN_PRIMITIVE: type = "primitive"; break;				
			}

			printf( 
				"%s: '%.*s' parent %d pos %d end %d size %d\n", 
					type,
					tt.end-tt.start, 
					full_string+tt.start, 
					tt.parent,
					tt.start, tt.end, tt.size
					 );


}

void json_dump(jsmntok_t * tokens, size_t count, const char *full_string)
{
		jsmntok_t *t = tokens;
		while(count-- > 0)
		{
			json_dump_token(t++, full_string);
		}
	
}

//! tokens - array of tokens to scan in
//! count - size of tokens
//! parent_token - token we are children of
//! level - descend level
//! size - number of children we're supposed to find
//!
//! returns number of tokens processed
size_t json_descent( 
	jsmntok_t *tokens, size_t count, 
	jsmntok_t *parent_token, int level, int size, int parent,
	//int shift, // what pos we start at
	const char *full_string )
{
	size_t done = 0;

	while( size-- > 0 )
	{
		int i = level;
		while(i--) printf("\t");
		json_dump_token(tokens,full_string);
		
		if( tokens->parent != parent )
		{
			printf("given parent %d, tokens->parent %d\n", parent, tokens->parent );
		}

		json_process_token(tokens,full_string, level);

		int current_pos = parent+done+1;

		if( tokens->size )
		{
			// have children
			size_t rc = json_descent( 
				tokens+1, count-done, 
				tokens+0, level+1, tokens->size, 
				current_pos,
				full_string );
			done += rc;
			tokens += rc;
		}

		done++;
		tokens++;
	}

	return done;
}

void parse_and_dump( const char * j1 )
{
	errno_t rc;
	jsmntok_t *tokens;
	size_t count = 0;

	rc = json_parse( j1, &tokens, &count );
	if( rc )
		printf("rc = %d for '%s'\n", rc, j1 );
	else
	{
		printf("parsed '%s', %d tokens\n\n", j1, count );
		json_dump(tokens,count,j1);
		printf("\n\n");
		json_descent( tokens+1, count-1, tokens+0, 0, tokens[0].size, 0, j1 );
		free(tokens);
	}
}

void pvm_json_test()
{
	sleep(1);
	printf("\n");

	//const char j1[] = "{ \"a\" : \"hello\", \"b\" : 122, \"c\" : [ \"c1\", \"c2\" ] }";
	//parse_and_dump(j1);

	const char j2[] = 
	"{"
	" \"object\" : { \"class\":\".internal.string\", \"value\" : \"test\" },"
	" \"object\" : { \"class\":\".internal.int\", \"value\" : 42 },"
	"}";

	parse_and_dump(j2);


}

#if 0
#include <vm/object.h>
#include <vm/p2c.h>
#include <vm/reflect.h>

static char *ep; // TODO killme

static const char *parse_string(char **item, const char *str);

/*

// TODO need some cache/hash or will be incredibly slow
static errno_t vm_field_ordinal_by_name( int *out, pvm_object_t oclass, const char *name )
{
	if( out == 0 )
		return ENOMEM;

	int sz = pvm_get_field_name_count( oclass );

	int ordinal = 0;
	for( ordinal = 0; ordinal < sz; ordinal++ )
	{
		pvm_object_t nm = pvm_get_field_name( oclass, ordinal );

		if( pvm_is_null(nm) || (!IS_PHANTOM_STRING(nm)))
			continue;

		if( EQ_STRING_P2C(nm,name) )
		{
			*out = ordinal;
			return 0;
		}
	}

	return ENOENT;

}

*/

static errno_t get_child( pvm_object_t *out, pvm_object_t parent, const char *name )
{
	if( pvm_is_null(parent) )
		return ENOTDIR;

	pvm_object_t oc = pvm_get_class( parent );


	int ord;
	errno_t rc = vm_field_ordinal_by_name( &ord, oc, name );

	int as = get_array_size(parent.data);


}

errno_t vm_json_parse( struct json_parse_context *jpc, pvm_object_t root, const char *value )
{

	if (*value!='{')	{ep=value;return 0;}	/* not an object! */

	//item->type=cJSON_Object;
	value=skip(value+1);
	if (*value=='}') return value+1;	/* empty array. */

	//item->child=child=cJSON_New_Item();	if (!item->child) return 0;
	char *item_name;
	value=skip(parse_string( &item_name,skip(value)));
	if (!value) return 0;

	//child->string=child->valuestring;child->valuestring=0;
	if (*value!=':') {ep=value;return 0;}	/* fail! */
	value=skip(parse_value(child,skip(value+1)));	/* skip any spacing, get the value. */
	if (!value) return 0;

	while (*value==',')
	{
		cJSON *new_item;
		if (!(new_item=cJSON_New_Item()))	return 0; /* memory fail */
		child->next=new_item;new_item->prev=child;child=new_item;
		value=skip(parse_string(child,skip(value+1)));
		if (!value) return 0;
		child->string=child->valuestring;child->valuestring=0;
		if (*value!=':') {ep=value;return 0;}	/* fail! */
		value=skip(parse_value(child,skip(value+1)));	/* skip any spacing, get the value. */
		if (!value) return 0;
	}

	if (*value=='}') return value+1;	/* end of array */
	ep=value;return 0;	/* malformed. */


}








/* Parse the input text into an unescaped cstring, and populate item. */
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
static const char *parse_string(char **item, const char *str)
{
	const char *ptr=str+1;char *ptr2;char *out;int len=0;unsigned uc,uc2;
	if (*str!='\"') {ep=str;return 0;}	/* not a string! */

	while (*ptr!='\"' && *ptr && ++len) if (*ptr++ == '\\') ptr++;	/* Skip escaped quotes. */

	out=(char*)malloc(len+1);	/* This is how long we need for the string, roughly. */
	if (!out) return 0;


	ptr=str+1;ptr2=out;
	while (*ptr!='\"' && *ptr)
	{
		if (*ptr!='\\') *ptr2++=*ptr++;
		else
		{
			ptr++;
			switch (*ptr)
			{
			case 'b': *ptr2++='\b';	break;
			case 'f': *ptr2++='\f';	break;
			case 'n': *ptr2++='\n';	break;
			case 'r': *ptr2++='\r';	break;
			case 't': *ptr2++='\t';	break;
			case 'u':	 /* transcode utf16 to utf8. */
				sscanf(ptr+1,"%4x",&uc);ptr+=4;	/* get the unicode char. */

				if ((uc>=0xDC00 && uc<=0xDFFF) || uc==0)	break;	// check for invalid.

				if (uc>=0xD800 && uc<=0xDBFF)	// UTF16 surrogate pairs.
				{
					if (ptr[1]!='\\' || ptr[2]!='u')	break;	// missing second-half of surrogate.
					sscanf(ptr+3,"%4x",&uc2);ptr+=6;
					if (uc2<0xDC00 || uc2>0xDFFF)		break;	// invalid second-half of surrogate.
					uc=0x10000 | ((uc&0x3FF)<<10) | (uc2&0x3FF);
				}

				len=4;if (uc<0x80) len=1;else if (uc<0x800) len=2;else if (uc<0x10000) len=3; ptr2+=len;

				switch (len) {
				case 4: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
				case 3: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
				case 2: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
				case 1: *--ptr2 =(uc | firstByteMark[len]);
				}
				ptr2+=len;
				break;
				default:  *ptr2++=*ptr; break;
			}
			ptr++;
		}
	}
	*ptr2=0;
	if (*ptr=='\"') ptr++;

	if(item)
		*item=out;

	return ptr;
}




#endif
