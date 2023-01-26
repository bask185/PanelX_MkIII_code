#include "src/macros.h"
#include "nx.h"


uint8_t indexTemp = 0 ;




void setup()
{
	Serial.begin(115200);
	Serial.println("booted") ;
}

void loop()
{
	REPEAT_MS( 2000 )
	{
		setNxButton( 1,     FIRST_BUTTON ) ;
		setNxButton( 4,  SECOND_BUTTON ) ;

		if( ++indexTemp ==  2 ) while(1);
	}
	END_REPEAT

	runNx() ;

}