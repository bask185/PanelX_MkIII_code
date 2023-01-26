#include "nx.h"
#include "src/macros.h"
#include "src/i2cEeprom.h"
#include "src/stateMachineClass.h"

/* STEPS TO MAKE
V todo get the street from EEPROM
* make a new branch and make project compilable
* remake the .ino file to work with the new functions.
*/


const uint8_t hardcodedRoutes[][2]
{
    {1,3},
    {1,2},

    {3,4},

    {2,5},
    {2,6},
    {2,7},
    {2,8},
    {2,9},
    {2,10},

    {10,11},
    {10,12},
    {10,13},
    {10,14},
    {10,15},
    {10,16},

    {5,17},
    {5,18},
    {5,19},
    {5,20},
    {5,21},
    {5,22},

    {21,23},
    {21,24},
    {21,25},
    {21,26},
    {21,27},
    {21,28},

    {24,29},
    {24,30},
    {24,31},
    {24,32},
    {24,33},
    {24,34},
} ;

#define entryState  if( sm.entryState() )
#define onState     if( sm.onState() )
#define exitState   if( sm.exitState() == 0 ) return 0 ; \
                    else

static StateMachine sm ;


enum states
{
    getBeginButton,
    getEndButton,
    findRoute,
    bufferNode,
    setPoints,    
} ;

const int   nBlocks = 33 ;
const int   nLevels =  6 ; // maybe 4 suffices?

uint8_t     state = getBeginButton ;
uint8_t     beginButton ;
uint8_t     endButton ;
uint8_t     street ;
uint8_t     relaisPresent ;
uint8_t     freeRouteOnTrain ;
uint8_t     directionMatters ;
int8_t      speed ;
uint16_t    point ;
uint8_t     relay ;
uint16_t    splits[nBlocks][nLevels] ;  // used to store the splits  10 possible tracks, 6 levels deep, should suffice
uint16_t    index[nLevels] ;            // keeps of track of the tried routes during route finding
uint8_t     level ;
uint16_t    sets[6][2] ;


I2cEeprom   eeprom( 0x50 ) ;

const int   nPointsPerStreet = 13 ;
const int   sizeAddress  = 0x6FFE ;
const int   startAddress = 0x6FFF ; // should be the last free 4096 bytes memory in a 24LC256 EEPROM

const int   NA   = 0xFF ;
const int   last = 10000 ;
const int   maxCombinations = 128 ;

struct
{
    uint8_t     beginButton ;
    uint8_t     endButton ;
    uint16_t    point[17] ;
    uint16_t    relay[5] ;
} Route;

uint8_t nButtons ;
uint8_t nStreets = 24 ;

void dumpTable()
{
    for( int j = 0 ; j < nLevels ; j ++ )
    {
        char buffer[60] ;
        sprintf(buffer, "%4d,%4d,%4d,%4d,%4d,%4d,%4d,%4d |%4d | %4d",  
            splits[0][j], 
            splits[1][j], 
            splits[2][j], 
            splits[3][j],
            splits[4][j], 
            splits[5][j], 
            splits[6][j], 
            splits[7][j],
            index[j],
            splits[index[j]][j] ) ;
        Serial.print(buffer) ;
        if( level == j ) Serial.println(F(" <-- current level"));
        else Serial.println() ; 
    }
}

void printResults()
{
    for( int j = 0 ; j <= level ; j ++ )
    {
        sets[j+1][0] = splits[index[j]][j] ;
        sets[j+1][1] = splits[index[j+1]][j+1] ;
        
        if( j == 0 )      {  sets[0][0] = beginButton ; sets[0][1] =  splits[index[0]][0] ; }
        if( j == level-1) {  sets[j+1][1] = endButton ; }

        Serial.print(j);Serial.print(": ") ;Serial.print(sets[j][0]);Serial.print(",") ;Serial.println(sets[j][1]) ;
    }
}

void NxBegin( uint8_t _relaisPresent, uint8_t _freeRouteOnTrain, uint8_t _directionMatters )
{
    relaisPresent    = _relaisPresent ;
    freeRouteOnTrain = _freeRouteOnTrain ;
    directionMatters = _directionMatters ;

    //nStreets = eeprom.read( sizeAddress) ;
    if( nStreets = 0xffff && invalidData ) invalidData() ; // if the size byte is larger than the the max amount of streets, warning should be displayed!
}

void storeRoutes( uint8_t *data, uint16_t size )
{
    uint16_t eeAddress = sizeAddress ;
    eeprom.write( eeAddress++, size ) ;

    for( int i = 0 ; i <  size ; i ++ )
    {
        eeprom.write( eeAddress++, *data ++ ) ;
    }
}


void setNxButton( uint8_t id, uint8_t val )
{
    if( val ==  FIRST_BUTTON ) { beginButton = id ; /* Serial.print(F("begin button set")) ;Serial.println(id);*/ }
    if( val == SECOND_BUTTON ) { endButton = id ; /*Serial.print(F("end button set")) ;Serial.println(id);*/ }
}

void setNxSpeed( int8_t _speed )
{
    speed = _speed ;
}

StateFunction( getBeginButton )
{
    entryState 
    {
        beginButton = NA ;
        endButton = NA ;
        Serial.println("\r\n\r\n\r\nwaiting  buttons") ;
    }
    if( beginButton != NA ) { sm.exit() ;/* Serial.print("begin button: ") ;Serial.println(beginButton);*/ }    

    return sm.endState() ;
}

StateFunction( getEndButton )
{
    entryState 
    {
       // Serial.println("waiting end button") ;
    }

    if(  beginButton == NA 
    ||  endButton != NA ) { sm.exit() ; /*Serial.print("end button: ") ;Serial.println(endButton);*/ }                              // if second button is also pressed, go find the street index

    return sm.endState() ;
}

/*



*/

StateFunction( findRoute )
{
    static uint8_t startButton ;
    static uint8_t buttonIndex ;
    uint8_t        nextTrackFound ;

    entryState
    {
        uint16_t eeAddress = startAddress ;  // hypothetically, only the numbers are store, I worry about points later on.
        level = 0 ;
        
        
        startButton = beginButton ;


        for( int i = 0 ; i < nBlocks ; i ++ )
        for( int j = 0 ; j < nLevels ; j ++ )
        {
            splits[i][j] = 0xffff ; // reset arrays
            index[j] = 0xffff ;
            index[ j ] = 0 ;
        }
    }
    onState
    {
        nextTrackFound = false ;
        Serial.print(F("Starting from: "));Serial.println(startButton);

        for( int i = 0 ; i < nBlocks ; i ++ )
        {
            //uint8_t btn1 = eeprom.read( eeAddress ++ ) ;
            //uint8_t btn2 = eeprom.read( eeAddress ++ ) ;

            uint8_t btn1 = hardcodedRoutes[i][0] ;
            uint8_t btn2 = hardcodedRoutes[i][1] ;

           //Serial.print(F("referencing "));Serial.print(startButton) ; Serial.write('/');Serial.print(endButton) ;
           //Serial.print(F(" with array "));Serial.print(btn1) ; Serial.write('/');Serial.print(btn2) ;
            if( btn1 == startButton )
            {
                // Serial.print(F("  storing ")) ; 
                // Serial.print( btn2 );Serial.print(F(" on index ")); 
                // Serial.print(index[level]);
                // Serial.print(F(" on level ")); Serial.println(level);
                splits[ index[level]++ ][level] = btn2 ; // if only the begin button matches, we want to store it.
                nextTrackFound = true ;
            }

            if( btn1 == startButton
            &&  btn2 == endButton ) // !! Route found, we are go!
            {       
                Serial.println(F("\r\nROUTE FOUND!")) ;
                dumpTable() ;
                printResults() ;                
                //beginButton = endButton = NA ;
                return 1 ;
            }
        }
        // NO route found, a or several blocks must be along the route
        // use one of the stored end positions as new begin position, and retry the same search 
    

        if( nextTrackFound == false ) // if no new possible track was found to go from...
        {
            Serial.println(F("DEAD END")) ; 
            level -- ;
            Serial.print(F("decrementing LEVEL to ")) ;Serial.println(level); // TO BE VERIFIED
            index[level] -- ;
            Serial.print(F("decrementing INDEX to ")) ;Serial.println(index[level]); // TO BE VERIFIED
           

            if( index[level] == -1  )
            {
                index[level] = 0 ; // maybe is -1 not bad?
                //index[level-1] -- ;
                Serial.println("depleted last branch, going back one level") ;
                // check if level = -1
                //Serial.println(F("explored last option, NO ROUTE FOUND"))  ;
            }
            else
            {   
                startButton = splits[ index[level]][level] ; // pick new block to start searching from      
                level ++ ;  
            }  
        }

        else // new tracks lie ahead THIS SHOULD BE OK
        {
            index[level] -- ;
            Serial.println(F("New split ahead, incrementing level")) ;
            startButton = splits[ index[level]][level] ; // pick new block to start searching from
            level ++ ;
        }

        dumpTable() ;
        Serial.println();
    }

    return 0 ;
}

StateFunction( bufferNode )
{
    entryState
    {
        
    }
    onState
    {
        
        sm.exit() ;
    }
    exitState
    {
        
        return 1 ;
    }
}

StateFunction( setPoints )
{
    entryState
    {
        
    }
    onState
    {
        
        sm.exit() ;
    }
    exitState
    {
        
        return 1 ;
    }
}

uint8_t runNx()
{
    STATE_MACHINE_BEGIN( sm )
    {
        State( getBeginButton ) {
            sm.nextState( getEndButton, 0 ) ; }

        State( getEndButton ) {
            if( beginButton == NA ) sm.nextState( getBeginButton, 0 ) ;
            else                    sm.nextState( findRoute, 0 ) ; }

        State( findRoute ) {
            sm.nextState( getBeginButton, 0 ) ; }
            /*sm.nextState( setPoints, 0  ) ;
            sm.nextState( bufferNode, 0 ) ; }*/

        State( setPoints ) {
            sm.nextState( getBeginButton, 0 ) ; }
    }
    STATE_MACHINE_END( sm )
}
/*
void runNx()    
{
    switch( state )
    {
    case getBeginButton:
        if( beginButton != NA ) { state = getEndButton ; }
        break ;

    case getEndButton:
        if(  beginButton == NA ) { state = getBeginButton ; }                   // if the first button is no longer pressed, go back
        if( endButton != NA ) { state = getIndex ; }                         // if second button is also pressed, go find the street index
        break ;

    case getIndex:                                                              // 2 buttons are found, go find out which street matches with these buttons 
        uint16_t eeAddress = startAddress ;
        for( int i = 0 ; i < nStreets ; i ++ )
        {
            eeprom.get( eeAddress, Route ) ;                                    // fetch route from EEPROM
            eeAddress += sizeof( Route ) ;

            if((  beginButton == Route.beginButton && endButton == Route.endButton )
            || ( endButton == Route.beginButton &&  beginButton == Route.endButton && directionMatters ))
            {
                index = 0 ;
                beginButton  = NA ;     // reset buttons
                endButton = NA ;
                state = setRoute ;
                return ;
            }
        }

        state = getBeginButton ;        
        break ;

    case setRoute:
        REPEAT_MS( 250 )
        {
            point = Route.point[index++] ; // get point from array

            if( point == last )  // finished
            {
                if( relaisPresent )
                {
                    state = setRelays ;
                }
                else if( freeRouteOnTrain )
                {
                    state = waitDepature ;   
                    if( routeSet ) routeSet() ;
                }
                else
                {
                    state = getBeginButton ; 
                    if( routeSet )   routeSet() ; 
                    if( routeFreed ) routeFreed() ;
                }
                break ;
            }

            else   // setting turnouts
            {
                uint16_t    address     = point & 0x03FF ;
                uint8_t     pointState  = point >> 15 ;  
                if( setNxTurnout ) setNxTurnout( address, pointState ) ;
            }
        }
        END_REPEAT
        break ;

    /*
    case setRelays:
        index = 0 ;
        for( int i = 0 ; i < nRelais ; i ++ )
        {
            if( setNxRelay ) setNxRelay( i, 0 ) ; // kill all relais before setting new ones
        }

        uint16_t Route.relay[index++] ;
        if( relay == last )     // finished
        {
            if( routeSet )
            {
                routeSet() ;
            }
            if( freeRouteOnTrain )
            {
                state = waitDepature ;
            }
            else
            {
                state = getBeginButton ; 
                if( routeFreed ) routeFreed() ;
            }
            break ;
        }

        else        // seting relays
        {
            uint16_t    address     = point & 0x03FF ;
            uint8_t     relayState  = point >> 15 ;  
            if( setNxRelay ) setNxRelay( address, relayState ) ;
        }
        break ;
    * /

    case waitDepature:
        if( speed != 0 ) state = waitArrival ;
        break ;

    case waitArrival:
        if( speed == 0 ) 
        {
            if( routeFreed ) routeFreed() ;
            state = getBeginButton ;
        }
        break ;
    }
}

*/