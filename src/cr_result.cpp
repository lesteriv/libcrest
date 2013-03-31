/**********************************************************************************************/
/* cr_result.cpp				                                               				  */
/*                                                                       					  */
/* Igor Nikitin, 2013																		  */
/* MIT license			                                                  					  */
/**********************************************************************************************/

// STD
#include <string.h>

// CREST
#include "../include/cr_connection.h"
#include "../include/cr_result.h"
#include "../include/cr_utils.h"
#include "../third/zlib/zlib.h"

/**********************************************************************************************/
using namespace std;


//////////////////////////////////////////////////////////////////////////
// static data
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static const string g_default_name	= "record";
static const string g_str_blob		= "blob";
static const string g_str_float		= "float";
static const string g_str_integer	= "integer";
static const string g_str_nil		= "nil";
static const string g_str_null		= "null";
static const string g_str_text		= "text";


//////////////////////////////////////////////////////////////////////////
// helper functions
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
static string escapeHTML( const string& html )
{
	string res;
	
	for( char ch : html )
	{
		switch( ch )
		{
            case '&'  : res += "&amp;"  ; break;
            case '\'' : res += "&apos;" ; break;
			case '"'  : res += "&quot;" ; break;
			case '<'  : res += "&lt;"   ; break;
			case '>'  : res += "&gt;"   ; break;
			case '\n' : res += "<BR>"   ; break;

            default:
				res.push_back( ch );
		}
	}

	return res;
}

/**********************************************************************************************/
static string escapeJSON( const string& json )
{
	string res;

	for( char ch : json )
	{
		switch( ch )
		{
            case '"'  : res += "\\\"" ; break;
            case '\\' : res += "\\\\" ; break;
            case '/'  : res += "\\/"  ; break;
            case '\b' : res += "\\b"  ; break;
            case '\f' : res += "\\f"  ; break;
            case '\n' : res += "\\n"  ; break;
            case '\r' : res += "\\r"  ; break;
            case '\t' : res += "\\t"  ; break;

            default:
				res.push_back( ch );
		}
	}

	return res;
}

/**********************************************************************************************/
static string escapeXML( const string& xml )
{
	string res;

	for( char ch : xml )
	{
		switch( ch )
		{
			case '<'	: res += "&lt;"	  ; break;
			case '>'	: res += "&gt;"	  ; break;
			case '\''	: res += "&apos;" ; break;
			case '"'	: res += "&quot;" ; break;
			case '&'	: res += "&amp;"  ; break;

            default:
			{
				if( ch < 32 )
					res += "&#" + to_string( ch ) + ";";
				else
					res.push_back( ch );
			}
		}
	}

	return res;
}

/**********************************************************************************************/
inline void pushInt32( string& s, int32_t n )
{
	s.push_back( n % 255 ); n >>= 8;
	s.push_back( n % 255 ); n >>= 8;
	s.push_back( n % 255 ); n >>= 8;
	s.push_back( n % 255 );
}

/**********************************************************************************************/
inline void pushInt64( string& s, int64_t n )
{
	pushInt32( s, n >> 32 );
	pushInt32( s, n & 0xFFFFFF );
}

/**********************************************************************************************/
inline void setInt32( string& s, int32_t i, int n )
{
	s[i++] = n % 255; n >>= 8;
	s[i++] = n % 255; n >>= 8;
	s[i++] = n % 255; n >>= 8;
	s[i  ] = n % 255;
}

/**********************************************************************************************/
const string& typeName( cr_value_type type )
{
	switch( type )
	{
		case CR_INTEGER	: return g_str_integer;
		case CR_FLOAT	: return g_str_float;
		case CR_TEXT	: return g_str_text;
		case CR_BLOB	: return g_str_blob;
	}
	
	return g_str_text;
}


//////////////////////////////////////////////////////////////////////////
// construction
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
cr_result::cr_result( cr_result_format format )
{
	format_ = format;
	init();
}

/**********************************************************************************************/
cr_result::cr_result( cr_connection& conn )
{
	format_ = cr_get_result_format( conn );
	init();
}

							
//////////////////////////////////////////////////////////////////////////
// methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
void cr_result::add_double( double value )
{
	switch( format_ )
	{
		case CR_FORMAT_BINARY	: data_.push_back( 1 ); pushInt64( data_, *(int64_t*) &value ); break;
		case CR_FORMAT_HTML		: add_text( to_string( value ) ); break;
		case CR_FORMAT_JSON		: add_text( to_string( value ) ); break;
		case CR_FORMAT_XML		: add_text( to_string( value ) );break;
		
		default:;
	}
}

/**********************************************************************************************/
void cr_result::add_int( int64_t value )
{
	switch( format_ )
	{
		case CR_FORMAT_BINARY	: data_.push_back( 1 ); pushInt64( data_, value ); break;
		case CR_FORMAT_HTML		: add_text( to_string( value ) ); break;
		case CR_FORMAT_JSON		: add_text( to_string( value ) ); break;
		case CR_FORMAT_XML		: add_text( to_string( value ) );break;
		
		default:;
	}
}

/**********************************************************************************************/
void cr_result::add_null( void )
{
	switch( format_ )
	{
		case CR_FORMAT_BINARY	: data_.push_back( 0 ); break; // TODO
		case CR_FORMAT_HTML		: add_text( g_str_null ); break;
		case CR_FORMAT_JSON		: add_text( g_str_nil ); break;
		case CR_FORMAT_XML		: add_text( g_str_null );break;
		
		default:;
	}
}

/**********************************************************************************************/
void cr_result::add_text( const string& value )
{
	// Start record
	if( column_ == 0 )
	{
		switch( format_ )
		{
			case CR_FORMAT_HTML:
			{
				data_ += "\t<tr>\n";
			}
			break;
			
			case CR_FORMAT_JSON:
			{
				if( records_ ) data_.push_back( ',' );
				data_.push_back( '\n' );

				if( has_properties_ ) data_.push_back( '\t' );
				data_ += "\t{\n";
			}
			break;
			
			case CR_FORMAT_XML:
			{
				if( has_properties_ ) data_.push_back( '\t' );
				data_ += "\t<" + *name_ + ">\n";
			}
			break;
			
			default:;
		}
		
		records_++;
	}
	
	// Add value
	switch( format_ )
	{
		case CR_FORMAT_BINARY:
		{
			data_.push_back( 1 ); 
			pushInt32( data_, value.length() );
			data_ += value;			
		}
		break;
		
		case CR_FORMAT_HTML:
		{
			const cr_result_field& fld = (*fields_)[ column_ ];
			
			( fld.type == CR_INTEGER || fld.type == CR_FLOAT ) ?
				data_ += "\t\t<td align=right>" + escapeHTML( value ) + "</td>\n" :
				data_ += "\t\t<td>" + escapeHTML( value ) + "</td>\n";
		}
		break;
			
		case CR_FORMAT_JSON:
		{
			const cr_result_field& fld = (*fields_)[ column_ ];
			if( has_properties_ ) data_.push_back( '\t' );
			data_ += "\t\t\"" + fld.name + "\": ";
			
			// TODO: get type as parameter
			if( fld.type == CR_INTEGER || fld.type == CR_FLOAT )
				data_ += value;
			else
				data_ += '"' + escapeJSON( value ) + '"';
			
			if( column_ < fields_->size() - 1 ) data_.push_back( ',' );
			data_.push_back( '\n' );
		}
		break;
		
		case CR_FORMAT_XML:
		{
			const cr_result_field& fld = (*fields_)[ column_ ];
			if( has_properties_ ) data_.push_back( '\t' );
			data_ += "\t\t<" + fld.name + ">"  + escapeXML( value ) + "</" + fld.name + ">\n";
		}
		break;
		
		default: break;
	}
	
	// Finish record
	if( ++column_ >= fields_->size() )
	{
		switch( format_ )
		{
			case CR_FORMAT_HTML:
			{
				data_ += "\t</tr>\n";
			}
			break;
			
			case CR_FORMAT_JSON:
			{
				if( has_properties_ ) data_.push_back( '\t' );
				data_ += "\t}";
			}
			break;
			
			case CR_FORMAT_XML:
			{
				if( has_properties_ ) data_.push_back( '\t' );
				data_ += "\t</" + *name_ + ">\n";
			}
			break;
			
			default:;
		}
		
		column_ = 0;
	}
}

/**********************************************************************************************/
void cr_result::add_property(
	const string&	key,
	const string&	value )
{
	switch( format_ )
	{
		case CR_FORMAT_BINARY:
		{
			properties_++;
			
			pushInt32( data_, key.length() );
			data_ += key;
			
			pushInt32( data_, value.length() );
			data_ += value;
		}
		break;
		
		case CR_FORMAT_HTML:
		{
			data_ += escapeHTML( key ) + " - " + escapeHTML( value ) + "<br>\n";
		}
		break;
		
		case CR_FORMAT_JSON:
		{
			bool dig = true;
			for( auto c : value )
			{
				if( !isdigit( c ) )
				{
					dig = false;
					break;
				}
			}
			
			data_.push_back( has_properties_ ? ',' : '{' );
			data_ += "\n\t\"" + key + "\": ";
			data_ += dig ? value : '"' + escapeJSON( value ) + '"';
		}
		break;
		
		case CR_FORMAT_XML:
		{
			data_ += "\t<" + key + ">" + escapeXML( value ) + "</" + key + ">\n";
		}
		break;
		
		default:;
	}
	
	has_properties_ = true;
}

/**********************************************************************************************/
string& cr_result::data( void )
{
	finish();
	return data_;
}

/**********************************************************************************************/
void cr_result::set_record_fields( const vector<cr_result_field>& fields )
{
	fields_ = &fields;
	
	switch( format_ )
	{
		case CR_FORMAT_BINARY:
		{
			for( auto& fld : fields )
			{
				data_.push_back( fld.type );			// Type
				pushInt32( data_, fld.name.length() );	// Field name length
				data_ += fld.name;						// Field name
			}				
		}
		break;
		
		case CR_FORMAT_HTML:
		{
			if( has_properties_ )
				data_ += "<br>\n";
			
			data_ += "<table>\n\t<tr>\n";
			
			for( auto& fld : fields )
				data_ += "\t\t<th>" + fld.name + "</th>\n";
					
			data_ += "\t<tr>\n";
		}
		break;
		
		case CR_FORMAT_JSON:
		{
			// Records
			if( has_properties_ )
				data_ += ",\n\t\"" + *name_ + "s\": [";
			else
				data_ = "[";
		}
		break;
		
		case CR_FORMAT_XML:
		{
			if( has_properties_ )
				data_ += "\t<" + *name_ + "s>\n";
		}
		break;
		
		default:;
	}
}

/**********************************************************************************************/
void cr_result::set_record_name( const string& name )
{
	name_ = &name;
}


//////////////////////////////////////////////////////////////////////////
// internal methods
//////////////////////////////////////////////////////////////////////////


/**********************************************************************************************/
void cr_result_internal::finish( void )
{
	if( finished_ ) return;
	finished_ = true;
	
	switch( format_ )
	{
		case CR_FORMAT_BINARY:
		{
			string res;
			res.resize( sizeof( cr_result_header ) );
			
			cr_result_header* header = (cr_result_header*) &res[ 0 ];
			header->protocol		= 0;
			header->flags			= 0;
			header->field_count		= fields_ ? fields_->size() : 0;
			header->property_count	= properties_;
			header->record_count	= records_;
			
			size_t len = data_.length();

			// Compress
			if( len > 1300 && 0 )
			{
				size_t out_len = cr_compress_bound( data_.length() );
				char*  out	   = (char*) malloc( out_len );
				
				out_len = cr_deflate( data_.c_str(), data_.length(), out, out_len );

				pushInt32( res, 0 );					// Packet size
				pushInt32( res, len );					// Uncompressed size
				pushInt32( res, records_ );				// Record count
				res += string( out, out_len );			// Compressed data
				
				free( out );
			}
			else
			{
				res += data_;
			}

			data_.swap( res );
		}
		break;
		
		case CR_FORMAT_HTML:
		{
			if( records_ )
				data_ += "</table>";
		}
		break;		
		
		case CR_FORMAT_JSON:
		{
			if( has_properties_ )
			{
				if( fields_ )
					data_ += "\n\t]";

				data_ += "\n}\n";
			}
			else
			{
				if( fields_ )
					data_ += "\n]\n";
			}
		}
		break;
		
		case CR_FORMAT_XML:
		{
			if( has_properties_ && records_ )
				data_ += "\t</" + *name_ + "s>\n";
			
			data_ += "</slite>\n";
		}
		break;
		
		default: break;
	}
}

/**********************************************************************************************/
void cr_result_internal::init( void )
{
	name_ = &g_default_name;
	
	data_.reserve( 512 );
	
	// Data header
	
	switch( format_ )
	{
		case CR_FORMAT_HTML:
			data_ = "<!doctype html><meta charset=utf-8><title></title>\n";
			break;
			
		case CR_FORMAT_XML:
			data_ = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<slite>\n";
			break;
		
		default:
			break;
	}
}
