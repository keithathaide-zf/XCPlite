/*----------------------------------------------------------------------------
| File:
|   A2L.c
|
| Description:
|   Generate A2L file
|   Linux (Raspberry Pi) Version
 ----------------------------------------------------------------------------*/


#include "A2L.h"

#define MAX_EVENT 256

FILE* gA2lFile = 0;
unsigned int gA2lEvent = 0;
unsigned int gA2lEventCount = 0;
char* gA2lEventList[MAX_EVENT];

char* gA2lHeader =
"/* generated by XCPlite */\n"
"ASAP2_VERSION 1 71\n"
"/begin PROJECT XCPlite \"\"\n"
"/begin HEADER \"\" VERSION \"1.0\" /end HEADER\n"
"/begin MODULE XCPlite \"\"\n"
"/include \"XCP_104.aml\"\n"
"/begin MOD_COMMON \"\"\n"
"BYTE_ORDER MSB_LAST\n"
"ALIGNMENT_BYTE 1\n"
"ALIGNMENT_WORD 1\n"
"ALIGNMENT_LONG 1\n"
"ALIGNMENT_FLOAT16_IEEE 1\n"
"ALIGNMENT_FLOAT32_IEEE 1\n"
"ALIGNMENT_FLOAT64_IEEE 1\n"
"ALIGNMENT_INT64 1\n"
"/end MOD_COMMON\n";

char* gA2lIfData1 =
"/begin IF_DATA XCP\n"
"/begin PROTOCOL_LAYER\n"
"0x0103 0x03E8 0x2710 0x00 0x00 0x00 0x00 0x00 0xFA 0x0574 BYTE_ORDER_MSB_LAST ADDRESS_GRANULARITY_BYTE\n"
"OPTIONAL_CMD GET_COMM_MODE_INFO\n"
"/end PROTOCOL_LAYER\n"
"/begin DAQ\n"
"DYNAMIC 0x00 0x03 0x00 OPTIMISATION_TYPE_DEFAULT ADDRESS_EXTENSION_FREE IDENTIFICATION_FIELD_TYPE_RELATIVE_BYTE GRANULARITY_ODT_ENTRY_SIZE_DAQ_BYTE 0xF8 OVERLOAD_INDICATION_PID\n"
"/begin TIMESTAMP_SUPPORTED\n"
"0x01 SIZE_DWORD UNIT_1US TIMESTAMP_FIXED\n"
"/end TIMESTAMP_SUPPORTED\n";

char* gA2lIfData2 = 
"/end DAQ\n"
"/begin PAG\n"
"0x00\n"
"/end PAG\n"
"/begin PGM\n"
"PGM_MODE_ABSOLUTE 0x00 0x00\n"
"/end PGM\n"
//"/begin XCP_ON_TCP_IP 0x0100 0x15B3 ADDRESS \"172.31.31.194\" /end XCP_ON_TCP_IP\n"
"/begin XCP_ON_UDP_IP 0x0103 0x15B3 ADDRESS \"172.31.31.195\" /end XCP_ON_UDP_IP\n"
"/end IF_DATA\n"
;

char* gA2lFooter =
"/end MODULE\n"
"/end PROJECT\n"
;

static const char* getParType(int size) {
	char* type;
	switch (size) {
	case -1: type = "_SBYTE";  break;
	case -2: type = "_SWORD";  break;
	case -4: type = "_SLONG";  break;
	case 1: type = "_UBYTE";   break;
	case 2: type = "_UWORD";   break;
	case 4: type = "_ULONG";   break;
	case 8: type = "_FLOAT64_IEEE";  break;
	default: type = NULL;
	}
	return type;
}

static const char* getMeaType(int size) {
	const char* type = getParType(size);
	if (type == NULL) return NULL;
	return &type[1];
}

static const char* getTypeMin(int size) {
	char* min;
	switch (size) {
	case -1: min = "-128";  break;
	case -2: min = "-32768"; break;
	case -4: min = "-2147483648";  break;
	case -8: min = "-1E12"; break;
	case 8: min = "-1E12"; break;
	default: min = "0";
	}
	return min;
}

static const char* getTypeMax(int size) {
	char* max;
	switch (size) {
	case -1: max = "127";  break;
	case -2: max = "32767";  break;
	case -4: max = "2147483647";  break;
	case -8: max = "1E12"; break;
	case 1: max = "255";  break;
	case 2: max = "65535";  break;
	case 4: max = "4294967295";  break;
	case 8: max = "1E12"; break;
	default: max = "1E12";
	}
	return max;
}



int A2lInit(const char *filename) {

	gA2lFile = 0;
	gA2lEvent = 0;
	gA2lEventCount = 0;
	gA2lFile = fopen(filename, "w");
	return gA2lFile != 0;
}


void A2lHeader(void) {

  assert(gA2lFile);
  fprintf(gA2lFile, gA2lHeader);
  fprintf(gA2lFile, gA2lIfData1);
  for (int i =0; i<gA2lEventCount; i++) fprintf(gA2lFile, "/begin EVENT \"%s\" \"%s\" 0x%X DAQ 0xFF 0x01 0x06 0x00 /end EVENT\n", gA2lEventList[i], gA2lEventList[i], i+1);
  fprintf(gA2lFile, gA2lIfData2);
}


void A2lCreateEvent(const char* name) {
	
	assert(gA2lEvent == 0);
	if (gA2lEventCount >= MAX_EVENT) return;
	gA2lEventList[gA2lEventCount] = (char*)name;
	gA2lEventCount++;
}


void A2lSetEvent(unsigned int event) {

	gA2lEvent = event;
}


void A2lCreateMeasurementType_(const char* name, int size, const char* comment) {

	fprintf(gA2lFile,
		"/begin TYPEDEF_STRUCTURE %s \"%s\" 0x%X SYMBOL_TYPE_LINK \"%s\"\n"
		"  /begin STRUCTURE_COMPONENT dummy_structure_component _ULONG 0 SYMBOL_TYPE_LINK \"dummy_symbol_type_link\" /end STRUCTURE_COMPONENT\n"
		"/end TYPEDEF_STRUCTURE\n"
			, name, comment, size, name);

}



void A2lCreateMeasurement_(const char* name, int size, unsigned long addr, double factor, double offset, const char* unit, const char* comment) {

	const char *conv = "NO";
	if (factor != 0.0 || offset != 0.0) {
		fprintf(gA2lFile, "/begin COMPU_METHOD %s_COMPU_METHOD \"\" LINEAR \"%6.3\" \"%s\" COEFFS_LINEAR %g %g /end COMPU_METHOD\n", name, unit!=NULL?unit:"", factor,offset);
		conv = name;
	}
	fprintf(gA2lFile,"/begin MEASUREMENT %s \"%s\" %s %s_COMPU_METHOD 0 0 %s %s ECU_ADDRESS 0x%X", name, comment, getMeaType(size), conv, getTypeMin(size), getTypeMax(size), addr);
	if (unit != NULL) fprintf(gA2lFile, " PHYS_UNIT \"%s\"", unit);
	if (gA2lEvent > 0) {
		fprintf(gA2lFile," /begin IF_DATA XCP /begin DAQ_EVENT FIXED_EVENT_LIST EVENT 0x%X /end DAQ_EVENT /end IF_DATA", gA2lEvent);
	}
	fprintf(gA2lFile, " /end MEASUREMENT\n");
}


void A2lCreateMeasurementArray_(const char* name, int size, int dim, unsigned long addr) {

	fprintf(gA2lFile,
		"/begin CHARACTERISTIC %s \"\" VAL_BLK 0x%X %s 0 NO_COMPU_METHOD %s %s MATRIX_DIM %u",
		name, addr, getParType(size), getTypeMin(size), getTypeMax(size), dim);
	if (gA2lEvent > 0) {
		fprintf(gA2lFile,
			" /begin IF_DATA XCP /begin DAQ_EVENT FIXED_EVENT_LIST EVENT 0x%X /end DAQ_EVENT /end IF_DATA",
			gA2lEvent);
	}
	fprintf(gA2lFile, " /end CHARACTERISTIC\n");
}


void A2lCreateParameter_(const char* name, int size, unsigned long addr, const char* comment, const char* unit) {

	fprintf(gA2lFile, "/begin CHARACTERISTIC %s \"%s\" VALUE 0x%X %s 0 NO_COMPU_METHOD %s %s PHYS_UNIT \"%s\" /end CHARACTERISTIC\n",
		name, comment, addr, getParType(size), getTypeMin(size), getTypeMax(size), unit);
}

void A2lCreateMap_(const char* name, int size, unsigned long addr, unsigned int xdim, unsigned int ydim, const char* comment, const char* unit) {

	fprintf(gA2lFile, 
		"/begin CHARACTERISTIC %s \"%s\" MAP 0x%X %s 0 NO_COMPU_METHOD %s %s"
		" /begin AXIS_DESCR FIX_AXIS NO_INPUT_QUANTITY NO_COMPU_METHOD  %u 0 %u FIX_AXIS_PAR_DIST 0 1 %u /end AXIS_DESCR"
		" /begin AXIS_DESCR FIX_AXIS NO_INPUT_QUANTITY NO_COMPU_METHOD  %u 0 %u FIX_AXIS_PAR_DIST 0 1 %u /end AXIS_DESCR"
		" PHYS_UNIT \"%s\" /end CHARACTERISTIC",
		name, comment, addr, getParType(size), getTypeMin(size), getTypeMax(size), xdim, xdim-1, xdim, ydim, ydim-1, ydim, unit);
}



void A2lCreateGroup(const char* name, int count, ...) {

	va_list ap;

	fprintf(gA2lFile, "/begin GROUP %s \"\"",name);
	fprintf(gA2lFile, " /begin REF_CHARACTERISTIC");
	va_start(ap, count);
	for (int i = 0; i < count; i++) {
		fprintf(gA2lFile, " %s", va_arg(ap, char*));
	}
	va_end(ap);
	fprintf(gA2lFile, " /end REF_CHARACTERISTIC");
	fprintf(gA2lFile, " /end GROUP\n");
}


void A2lClose(void) {

	// Create standard record layouts for elementary types
	for (int i = -8; i <= +8; i++) {
		const char* t = getMeaType(i);
		if (t != NULL) fprintf(gA2lFile, "/begin RECORD_LAYOUT _%s FNC_VALUES 1 %s ROW_DIR DIRECT /end RECORD_LAYOUT\n", t, t);
	}

	// Create standard typedefs for elementary types
	for (int i = -8; i <= +8; i++) {
		const char* t = getMeaType(i);
		if (t != NULL) fprintf(gA2lFile, "/begin TYPEDEF_MEASUREMENT _%s \"\" %s NO_COMPU_METHOD 0 0 %s %s /end TYPEDEF_MEASUREMENT\n",t,t,getTypeMin(i),getTypeMax(i));
	}

	fprintf(gA2lFile, gA2lFooter);
	fclose(gA2lFile);
}
