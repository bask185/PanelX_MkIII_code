#include "src/macros.h"
#include "nx.h"


uint8_t indexTemp = 0 ;

const uint8_t array[][2] =
{
	//{1,3},
	{1,6},
	{1,11},
	//{2,3},
}	;



void setup()
{
	Serial.begin(115200);
	Serial.println("booted") ;
}

void loop()
{
	REPEAT_MS( 500 )
	{
		setNxButton( array[indexTemp  ][0],     FIRST_BUTTON ) ;
		setNxButton( array[indexTemp++][1],  SECOND_BUTTON ) ;

		if( indexTemp ==  3 ) while(1);
	}
	END_REPEAT

	runNx() ;

}