#include <sl.h>
#include <push.h>

#include "Nvidia\nvapi.h"


int numTempPoints;
int numColumns;
int **arr;
int AmbientTemp;


void FreeSpeedMap()
{
	for (int i = 0; i < numTempPoints; i++)
	{
		Memory_Free(arr[i]);
	}

	Memory_Free(arr);

	arr = NULL;
}


void DrawSpeedMap( UINT32 MinTemp )
{
	int fanHigh = 80;
	int fanLow = 30;
	int tempMax = 80;
	//int tempMin = 24;
	int numFanPoints;
	int temp;
	float fanIncrease;
	float fanSpeed;

	numFanPoints = fanHigh - fanLow;
	numTempPoints = tempMax - MinTemp;
	fanIncrease = (float)numFanPoints / (float)numTempPoints;

	fanSpeed = (float)fanLow;
	temp = MinTemp;

	if (arr != NULL)
	{
		Log(L"Destroying speed map");
		FreeSpeedMap();
	}

	arr = Memory_Allocate((numTempPoints + 1) * sizeof(int *));
	numColumns = 2;

	/*if (arr == NULL)
	{
		Log(L"out of memory");
		return;
	}*/

	for (int i = 0; i < numTempPoints + 1; i++)
	{
		arr[i] = Memory_Allocate(numColumns * sizeof(int));
	}

	int j = 0;

	for (int i = 0; i < numTempPoints + 1; i++)
	{
		for (j = 0; j < numColumns; j++)
		{
			arr[i][0] = temp;
			arr[i][1] = (int)fanSpeed;
		}

		fanSpeed += fanIncrease;
		temp++;
	}

	/*for (int i = 0; i < numTempPoints+1; i++)
	{
		Log(L"[temp] = %i [fanSpeed] = %i", arr[i][0], arr[i][1]);
	}*/
}


void InitializeFanSettings()
{
	AmbientTemp = Nvapi_GetTemperature();

	DrawSpeedMap(AmbientTemp);
}


UINT32 GetSpeedFromMap( UINT32 Temp )
{
	for (int i = 0; i < numTempPoints + 1; i++)
	{
		for (int j = 0; j < numColumns; j++)
		{
			if (arr[i][0] == Temp)
			{
				return arr[i][1];
			}
		}
	}

	return 80;
}


void UpdateFanSpeed()
{
	int temp;
	int fanSpeed;
	int speedForTemp;

	temp = Nvapi_GetTemperature();

	//draw a new speed map every time a lower ambient temp is
	//discovered.
	if (temp < AmbientTemp)
	{
		AmbientTemp = temp;
		DrawSpeedMap(AmbientTemp);
	}

	fanSpeed = Nvapi_GetFanDutyCycle();
	speedForTemp = GetSpeedFromMap(temp);

	//Log(L"temp %i, fan %i, sft %i", temp, fanSpeed, speedForTemp);

	if (fanSpeed != speedForTemp)
	{
		Nvapi_SetFanDutyCycle(speedForTemp);
	}
}