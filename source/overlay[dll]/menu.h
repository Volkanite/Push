VOID Menu_Render( OvOverlay* Overlay );
VOID Menu_KeyboardHook( WPARAM Key );
BOOLEAN Menu_IsVisible();

typedef enum _OVERCLOCK_UNIT
{
    OC_ENGINE,
    OC_MEMORY,
    OC_VOLTAGE

}OVERCLOCK_UNIT;

typedef enum _CLOCK_STEP_DIRECTION
{
    Up,
    Down

}CLOCK_STEP_DIRECTION;

VOID ClockStep(OVERCLOCK_UNIT Unit, CLOCK_STEP_DIRECTION Direction);
