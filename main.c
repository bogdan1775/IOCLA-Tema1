#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "operations.h"
#include "structs.h"
#define NMAX 100

void verificare_alocare(void *param)
{
	if (param == NULL) {
		fprintf(stderr, "Alocare esuata");
		exit(-17);
	}
}

// se citesc senzorii din fisier
sensor *citire_valori(char *nume_fisier, int *n)
{
	// se deschide fisierul
	FILE *in = fopen(nume_fisier, "rb");
	verificare_alocare(in);

	sensor *vect;
	int size, tip;

	//se citeste numarul de senzori
	fread(&size, sizeof(int), 1, in);
	*n = size;
	vect = malloc(size * sizeof(sensor));
	verificare_alocare(vect);

	for (int i = 0; i < size; i++) {
		fread(&tip, sizeof(int), 1, in);
		if (tip == 1) {
			// se  citesc datele senzorului de tip PMU
			vect[i].sensor_type = PMU;
			power_management_unit *data;
			data = malloc(sizeof(power_management_unit));
			verificare_alocare(data);
			fread(&data->voltage, sizeof(float), 1, in);
			fread(&data->current, sizeof(float), 1, in);
			fread(&data->power_consumption, sizeof(float), 1, in);
			fread(&data->energy_regen, sizeof(int), 1, in);
			fread(&data->energy_storage, sizeof(int), 1, in);
			vect[i].sensor_data = data;

		} else {
			// se  citesc datele senzorului de tip TIRE
			vect[i].sensor_type = TIRE;
			tire_sensor *data = malloc(sizeof(tire_sensor));
			verificare_alocare(data);
			fread(&data->pressure, sizeof(float), 1, in);
			fread(&data->temperature, sizeof(float), 1, in);
			fread(&data->wear_level, sizeof(int), 1, in);
			fread(&data->performace_score, sizeof(int), 1, in);
			vect[i].sensor_data = data;
		}

		// se citeste numarul de operatii si vectorul de operatii
		fread(&vect[i].nr_operations, sizeof(int), 1, in);

		vect[i].operations_idxs = malloc(vect[i].nr_operations * sizeof(int));
		verificare_alocare(vect[i].operations_idxs);

		fread(vect[i].operations_idxs, sizeof(int), vect[i].nr_operations, in);
	}

	// se ordoneaza vectorul de senzori
	int poz = 0;
	for (int i = 0; i < size; i++)
		if (vect[i].sensor_type == PMU) {
			sensor *sensor_aux = malloc(sizeof(sensor));
			verificare_alocare(sensor_aux);

			memcpy(sensor_aux, &vect[i], sizeof(sensor));
			for (int j = i; j > poz; j--)
				memcpy(&vect[j], &vect[j - 1], sizeof(sensor));
			memcpy(&vect[poz++], sensor_aux, sizeof(sensor));
			free(sensor_aux);
		}

	// se inchide fisierul
	fclose(in);

	return vect;
}

// se afiseaza datele unui anumit senzor
void print_sensor(int index, sensor *vect, int n)
{
	// se verifica daca indexul este valid
	if (index < 0 || index >= n) {
		printf("Index not in range!\n");
		return;
	}

	if (vect[index].sensor_type == TIRE) {
		// se afiseaza datele senzorului de tip TIRE
		tire_sensor *data = vect[index].sensor_data;
		printf("Tire Sensor\n");
		printf("Pressure: %.2f\n", data->pressure);
		printf("Temperature: %.2f\n", data->temperature);

		char c = '%';
		printf("Wear Level: %d%c\n", data->wear_level, c);
		if (data->performace_score == 0)
			printf("Performance Score: Not Calculated\n");
		else
			printf("Performance Score: %d\n", data->performace_score);

	} else {
		// se afiseaza datele senzorului de tip PMU
		power_management_unit *data = vect[index].sensor_data;
		printf("Power Management Unit\n");
		printf("Voltage: %.2f\n", data->voltage);
		printf("Current: %.2f\n", data->current);
		printf("Power Consumption: %.2f\n", data->power_consumption);

		char c = '%';
		printf("Energy Regen: %d%c\n", data->energy_regen, c);
		printf("Energy Storage: %d%c\n", data->energy_storage, c);
	}
}

// se realizeaza operatiile din vectorul de operatii
void analyze(sensor *vect, int index, int n)
{
	// se verifica daca indexul este valid
	if (index < 0 || index >= n) {
		printf("Index not in range!\n");
		return;
	}

	// se aloca vectorul operations prin care se apeleaza cele 8 operatii
	void (**operations)(void *);
	operations = malloc(8 * sizeof(void *));
	verificare_alocare(operations);

	// se initializeaza
	get_operations((void *)operations);

	// se aplica operatiile pentru senzorul de pe pozitia index
	for (int i = 0; i < vect[index].nr_operations; i++) {
		operations[vect[index].operations_idxs[i]](vect[index].sensor_data);
	}

	free(operations);
}

// functia verifica daca senzorul de tip TIRE are datele bune
// returneaza 1 daca cel putin o valoare nu e buna
// altfel returneaza 0
int verificare_tire(sensor sensor)
{
	tire_sensor *data = sensor.sensor_data;
	int ok = 0;
	if (data->pressure < 19 || data->pressure > 28)
		ok = 1;

	if (data->temperature < 0 || data->temperature > 120)
		ok = 1;

	if (data->wear_level < 0 || data->wear_level > 100)
		ok = 1;

	return ok;
}

// functia verifica daca senzorul de tip PMU are datele bune
// returneaza 1 daca cel putin o valoare nu e buna
// altfel returneaza 0
int verificare_power_mang_unit(sensor sensor)
{
	power_management_unit *data = sensor.sensor_data;
	int ok = 0;

	if (data->voltage < 10 || data->voltage > 20)
		ok = 1;

	if (data->current < -100 || data->current > 100)
		ok = 1;

	if (data->power_consumption < 0 || data->power_consumption > 1000)
		ok = 1;

	if (data->energy_regen < 0 || data->energy_regen > 100)
		ok = 1;

	if (data->energy_regen < 0 || data->energy_storage > 100)
		ok = 1;

	return ok;
}

// functia sterge senzorii care au valori eronate
void clear(sensor **vect2, int *nr_senzori)
{
	sensor *vect = *vect2;
	int n = *nr_senzori;

	// se verifica fiecare senzor
	for (int i = 0; i < n; i++) {
		// status retine daca senzorul are valori eronate
		// 1 inseamna ca are valori eronate, 0 are valori bune
		int status;
		if (vect[i].sensor_type == TIRE)
			status = verificare_tire(vect[i]);
		else
			status = verificare_power_mang_unit(vect[i]);

		// se sterge senzorul care are valori eronate
		if (status == 1) {
			sensor sensor1 = vect[i];
			for (int j = i; j < n - 1; j++)
				vect[j] = vect[j + 1];

			// se scade numarul de senzori
			n--;
			vect[n] = sensor1;
			free(sensor1.sensor_data);
			free(sensor1.operations_idxs);

			// se da realloc vectorului de senzori
			vect = realloc(vect, n * sizeof(sensor));
			i--;
		}
	}

	*nr_senzori = n;
	*vect2 = vect;
}

// functia elibereaza toata memoria
void eliberare_memorie(sensor *vect, int n)
{
	// se elibereaza pentru fiecare senzor datele alocate dinamic
	for (int i = 0; i < n; i++) {
		free(vect[i].sensor_data);
		free(vect[i].operations_idxs);
	}

	// se elibereaza vectorul de senzori
	free(vect);
}

int main(int argc, char const *argv[])
{
	sensor *vect;
	int nr_senzori;

	// se apeleaza functia care citeste senzorii din fisier
	// se retine in nr_senzori numarul de senzori
	vect = citire_valori((char *)argv[1], &nr_senzori);
	char comanda[NMAX];

	while (1) {
		scanf("%s", comanda);

		if (strcmp(comanda, "print") == 0) {
			int index;
			scanf("%d", &index);
			print_sensor(index, vect, nr_senzori);

		} else if (strcmp(comanda, "analyze") == 0) {
			int index;
			scanf("%d", &index);
			analyze(vect, index, nr_senzori);

		} else if (strcmp(comanda, "clear") == 0) {
			int index;
			scanf("%d", &index);
			clear(&vect, &nr_senzori);

		} else if (strcmp(comanda, "exit") == 0) {
			eliberare_memorie(vect, nr_senzori);
			break;
		}
	}
	return 0;
}
