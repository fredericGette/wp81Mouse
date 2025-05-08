#pragma once

int bleConnectionStart(ConnectionStatus *pConnectionStatus, BYTE *pButton, int16_t *pX, int16_t *pY, int16_t *pWheel, int16_t *pHwheel, HANDLE hEventSendNotification);
void bleConnectionStop();