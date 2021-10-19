#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>


#pragma pack(1)

/*
Para compilar:
1 - Abrir o local do fonte
2 - Digitar para compilar: gcc main.c -o main -lm -pthread
3 - Digitar para rodar: ./main borboleta.bmp saida.bmp 3 1
*/

struct cabecalho {
	unsigned short tipo;
	unsigned int tamanho_arquivo;
	unsigned short reservado1;
	unsigned short reservado2;
	unsigned int offset;
	unsigned int tamanho_image_header;
	int largura;
	int altura;
	unsigned short planos;
	unsigned short bits_por_pixel;
	unsigned int compressao;
	unsigned int tamanho_imagem;
	int largura_resolucao;
	int altura_resolucao;
	unsigned int numero_cores;
	unsigned int cores_importantes;
};
typedef struct cabecalho CABECALHO;

struct rgb{
	unsigned char blue;
	unsigned char green;
	unsigned char red;
};
typedef struct rgb RGB;

struct parametros{
	int id;
	int nthr;
	int mascara;
	int range;
	CABECALHO cabecalho;
	RGB *imagemEntrada;
	RGB *imagemSaida;
	RGB *imagemCinza;
	RGB *imagemGaussiano;
};
typedef struct parametros PARAMETROS;

double tempoCorrente(void){
     struct timeval tval;
     gettimeofday(&tval, NULL);
     return (tval.tv_sec + tval.tv_usec/1000000.0);
}

void *filtroGaussiano(void *args) {
	PARAMETROS *par = (PARAMETROS *)args;
	int id = par->id;
	int nthr = par->nthr ;
	int mascara = par->mascara;
	int range = par->range;
	CABECALHO cabecalho = par->cabecalho;
	RGB *imagemCinza = par->imagemCinza;
	RGB *imagemGaussiano = par->imagemGaussiano;
	//------------------------------------------
	int iForGaussiano, jForGaussiano;
	int iForImagem, jForImagem;
	int iIni, iFim;
	int jIni, jFim;
	int iTamAux;
	int iPosMatriz;
	int matrizGaussiano[7][7];
	RGB *imagemAux, *imagemAuxGau;

	imagemAux 		= (RGB *)malloc((mascara*mascara)*sizeof(RGB));
	imagemAuxGau 	= (RGB *)malloc(1*sizeof(RGB));

	if (mascara == 3) {
		matrizGaussiano[0][0] = 1;
		matrizGaussiano[0][1] = 2;
		matrizGaussiano[0][2] = 1;

		matrizGaussiano[1][0] = 2;
		matrizGaussiano[1][1] = 4;
		matrizGaussiano[1][2] = 2;

		matrizGaussiano[2][0] = 1;
		matrizGaussiano[2][1] = 2;
		matrizGaussiano[2][2] = 1;
	}
	else if (mascara == 5) {
		matrizGaussiano[0][0] = 1;
		matrizGaussiano[0][1] = 4;
		matrizGaussiano[0][2] = 7;
		matrizGaussiano[0][3] = 4;
		matrizGaussiano[0][4] = 1;

		matrizGaussiano[1][0] = 4;
		matrizGaussiano[1][1] = 16;
		matrizGaussiano[1][2] = 26;
		matrizGaussiano[1][3] = 16;
		matrizGaussiano[1][4] = 4;

		matrizGaussiano[2][0] = 7;
		matrizGaussiano[2][1] = 26;
		matrizGaussiano[2][2] = 41;
		matrizGaussiano[2][3] = 26;
		matrizGaussiano[2][4] = 7;

		matrizGaussiano[3][0] = 4;
		matrizGaussiano[3][1] = 16;
		matrizGaussiano[3][2] = 26;
		matrizGaussiano[3][3] = 16;
		matrizGaussiano[3][4] = 4;

		matrizGaussiano[4][0] = 1;
		matrizGaussiano[4][1] = 4;
		matrizGaussiano[4][2] = 7;
		matrizGaussiano[4][3] = 4;
		matrizGaussiano[4][4] = 1;
	}
	else {
		matrizGaussiano[0][0] = 0;
		matrizGaussiano[0][1] = 0;
		matrizGaussiano[0][2] = 1;
		matrizGaussiano[0][3] = 2;
		matrizGaussiano[0][4] = 1;
		matrizGaussiano[0][5] = 0;
		matrizGaussiano[0][6] = 0;

		matrizGaussiano[1][0] =  0;
		matrizGaussiano[1][1] =  3;
		matrizGaussiano[1][2] = 13;
		matrizGaussiano[1][3] = 22;
		matrizGaussiano[1][4] = 13;
		matrizGaussiano[1][5] =  3;
		matrizGaussiano[1][6] =  0;

		matrizGaussiano[2][0] =  1;
		matrizGaussiano[2][1] = 13;
		matrizGaussiano[2][2] = 59;
		matrizGaussiano[2][3] = 97;
		matrizGaussiano[2][4] = 59;
		matrizGaussiano[2][5] = 13;
		matrizGaussiano[2][6] =  1;

		matrizGaussiano[3][0] =  2;
		matrizGaussiano[3][1] = 22;
		matrizGaussiano[3][2] = 97;
		matrizGaussiano[3][3] = 159;
		matrizGaussiano[3][4] = 97;
		matrizGaussiano[3][5] = 22;
		matrizGaussiano[3][6] =  2;

		matrizGaussiano[4][0] =  1;
		matrizGaussiano[4][1] = 13;
		matrizGaussiano[4][2] = 59;
		matrizGaussiano[4][3] = 97;
		matrizGaussiano[4][4] = 59;
		matrizGaussiano[4][5] = 13;
		matrizGaussiano[4][6] =  1;

		matrizGaussiano[5][0] =  0;
		matrizGaussiano[5][1] =  3;
		matrizGaussiano[5][2] = 13;
		matrizGaussiano[5][3] = 22;
		matrizGaussiano[5][4] = 13;
		matrizGaussiano[5][5] =  3;
		matrizGaussiano[5][6] =  0;

		matrizGaussiano[6][0] = 0;
		matrizGaussiano[6][1] = 0;
		matrizGaussiano[6][2] = 1;
		matrizGaussiano[6][3] = 2;
		matrizGaussiano[6][4] = 1;
		matrizGaussiano[6][5] = 0;
		matrizGaussiano[6][6] = 0;
	}

	//Aplicar filtro Gaussiano
	for(iForImagem=range+id; iForImagem<(cabecalho.altura-range); iForImagem+=nthr){
		for(jForImagem=range; jForImagem<(cabecalho.largura-range); jForImagem++){
			//Variaveis auxiliares
			iTamAux = 0;
			iIni = (iForImagem - range);
			iFim = (iForImagem + range);
			jIni = (jForImagem - range);
			jFim = (jForImagem + range);

			//Buscar valores da imagem em cinza para o calculo Gaussiano
			for (iForGaussiano = iIni; iForGaussiano <= iFim; iForGaussiano++)
            {
                for (jForGaussiano = jIni; jForGaussiano <= jFim; jForGaussiano++)
                {
					iPosMatriz = iForGaussiano * cabecalho.largura + jForGaussiano;

					imagemAux[iTamAux].red   = imagemCinza[iPosMatriz].red;
					imagemAux[iTamAux].green = imagemCinza[iPosMatriz].green;
					imagemAux[iTamAux].blue  = imagemCinza[iPosMatriz].blue;

					iTamAux++;
                }
            }

			//Variaveis auxiliares
			iTamAux = 0;
			imagemAuxGau[0].red   = 0;
			imagemAuxGau[0].green = 0;
			imagemAuxGau[0].blue  = 0;

			//Calcula o gaussiano
			for (iForGaussiano = 0; iForGaussiano < mascara; iForGaussiano++)
            {
                for (jForGaussiano = 0; jForGaussiano < mascara; jForGaussiano++)
                {
					imagemAuxGau[0].red   = imagemAuxGau[0].red   + (matrizGaussiano[iForGaussiano][jForGaussiano] * imagemAux[iTamAux].red);
					imagemAuxGau[0].green = imagemAuxGau[0].green + (matrizGaussiano[iForGaussiano][jForGaussiano] * imagemAux[iTamAux].green);
					imagemAuxGau[0].blue  = imagemAuxGau[0].blue  + (matrizGaussiano[iForGaussiano][jForGaussiano] * imagemAux[iTamAux].blue);

					iTamAux++;
                }
            }

			//Atualiza a imagem nova
			iPosMatriz = iForImagem * cabecalho.largura + jForImagem;
			
			imagemGaussiano[iPosMatriz].red   = imagemAux[0].red   / (mascara*mascara);
			imagemGaussiano[iPosMatriz].green = imagemAux[0].green / (mascara*mascara);
			imagemGaussiano[iPosMatriz].blue  = imagemAux[0].blue  / (mascara*mascara);
		}
	}
}

void filtroSobel(RGB *imagemGaussiano, RGB *imagemSaida, CABECALHO cabecalho, int range){
	int iForImagem, jForImagem;
	int iPosLinhaAnt, jPosColunaAnt;
	int iPosLinha, jPosColuna;
	int iPosLinhaPrx, jPosColunaPrx;
	int valorX, valorY;
	int iPosMatriz;
	int MascaraX[3*3], MascaraY[3*3];

	//MascaraX
	MascaraX[0] = -1; //P1
	MascaraX[1] =  0; //P2
	MascaraX[2] =  1; //P3

	MascaraX[3] = -2; //P4
	MascaraX[4] =  0; //P5 - Central
	MascaraX[5] =  2; //P6

	MascaraX[6] = -1; //P7
	MascaraX[7] =  0; //P8
	MascaraX[8] =  1; //P9

	//MascaraY
	MascaraY[0] = -1; //P1
	MascaraY[1] = -2; //P2
	MascaraY[2] = -1; //P3

	MascaraY[3] =  0; //P4
	MascaraY[4] =  0; //P5 - Central
	MascaraY[5] =  0; //P6

	MascaraY[6] =  1; //P7
	MascaraY[7] =  2; //P8
	MascaraY[8] =  1; //P9

	for(iForImagem=range; iForImagem<(cabecalho.altura-range); iForImagem++){
		for(jForImagem=range; jForImagem<(cabecalho.largura-range); jForImagem++){
			//Calcular as posicoes
			iPosLinhaAnt  = (iForImagem-1) * cabecalho.largura;
			jPosColunaAnt = jForImagem-1;

			iPosLinha  = (iForImagem) * cabecalho.largura;
			jPosColuna = jForImagem;

			iPosLinhaPrx  = (iForImagem+1) * cabecalho.largura;
			jPosColunaPrx = jForImagem+1;

			//Mascaras de sobel
			valorX   = MascaraX[0] * imagemGaussiano[(iPosLinhaAnt) + (jPosColunaAnt)].red
					 + MascaraX[1] * imagemGaussiano[(iPosLinhaAnt) + (jPosColuna)].red
					 + MascaraX[2] * imagemGaussiano[(iPosLinhaAnt) + (jPosColunaPrx)].red
					 + MascaraX[3] * imagemGaussiano[(iPosLinha)    + (jPosColunaAnt)].red
					 + MascaraX[4] * imagemGaussiano[(iPosLinha)    + (jPosColuna)].red
					 + MascaraX[5] * imagemGaussiano[(iPosLinha)    + (jPosColunaPrx)].red
					 + MascaraX[6] * imagemGaussiano[(iPosLinhaPrx) + (jPosColunaAnt)].red
					 + MascaraX[7] * imagemGaussiano[(iPosLinhaPrx) + (jPosColuna)].red
					 + MascaraX[8] * imagemGaussiano[(iPosLinhaPrx) + (jPosColunaPrx)].red; 

			valorY   = MascaraY[0] * imagemGaussiano[(iPosLinhaAnt) + (jPosColunaAnt)].red
					 + MascaraY[1] * imagemGaussiano[(iPosLinhaAnt) + (jPosColuna)].red
					 + MascaraY[2] * imagemGaussiano[(iPosLinhaAnt) + (jPosColunaPrx)].red
					 + MascaraY[3] * imagemGaussiano[(iPosLinha)    + (jPosColunaAnt)].red
					 + MascaraY[4] * imagemGaussiano[(iPosLinha)    + (jPosColuna)].red
					 + MascaraY[5] * imagemGaussiano[(iPosLinha)    + (jPosColunaPrx)].red
					 + MascaraY[6] * imagemGaussiano[(iPosLinhaPrx) + (jPosColunaAnt)].red
					 + MascaraY[7] * imagemGaussiano[(iPosLinhaPrx) + (jPosColuna)].red
					 + MascaraY[8] * imagemGaussiano[(iPosLinhaPrx) + (jPosColunaPrx)].red;
	
			//Imagem de saida		
			iPosMatriz = iPosLinha + jForImagem;

			imagemSaida[iPosMatriz].red    = sqrt(pow(valorX,2) + pow(valorY,2));
			imagemSaida[iPosMatriz].green  = sqrt(pow(valorX,2) + pow(valorY,2));
			imagemSaida[iPosMatriz].blue   = sqrt(pow(valorX,2) + pow(valorY,2));
		}
	}
}

void filtroCinza(RGB *imagemEntrada, RGB *imagemCinza, CABECALHO cabecalho) {
	int iForImagem, jForImagem;
	int iPosMatriz;

	for(iForImagem=0; iForImagem<cabecalho.altura; iForImagem++){
		for(jForImagem=0; jForImagem<cabecalho.largura; jForImagem++){
			iPosMatriz = iForImagem * cabecalho.largura + jForImagem;

			imagemCinza[iPosMatriz].red    = (0.2126 * imagemEntrada[iPosMatriz].red) + (0.7152 * imagemEntrada[iPosMatriz].green) + (0.0722 * imagemEntrada[iPosMatriz].blue);
			imagemCinza[iPosMatriz].green  = (0.2126 * imagemEntrada[iPosMatriz].red) + (0.7152 * imagemEntrada[iPosMatriz].green) + (0.0722 * imagemEntrada[iPosMatriz].blue);
			imagemCinza[iPosMatriz].blue   = (0.2126 * imagemEntrada[iPosMatriz].red) + (0.7152 * imagemEntrada[iPosMatriz].green) + (0.0722 * imagemEntrada[iPosMatriz].blue);
		}
	}
}

void escreverImagem(RGB *imagemSaida, CABECALHO cabecalho, FILE *fout) {
	int iForImagem, jForImagem;
	char ali, aux;

	//Escrever cabecalho saida
	fwrite(&cabecalho, sizeof(CABECALHO), 1, fout);

	//Escrever a imagem saida
	for(iForImagem=0; iForImagem<cabecalho.altura; iForImagem++){
		ali = (cabecalho.largura * 3) % 4;

		if (ali != 0){
			ali = 4 - ali;
		}

		for(jForImagem=0; jForImagem<cabecalho.largura; jForImagem++){
			fwrite(&imagemSaida[iForImagem * cabecalho.largura + jForImagem], sizeof(RGB), 1, fout);
		}

		for(jForImagem=0; jForImagem<ali; jForImagem++){
			fwrite(&aux, sizeof(unsigned char), 1, fout);
		}
	}
}

// void * filtroCinza(void *args){
// 	PARAMETROS *par = (PARAMETROS *)args;
// 	int id = par->id;
// 	int nthr = par->nthr ;
// 	int mascara = par->mascara;
// 	int range = par->range;
// 	CABECALHO cabecalho = par->cabecalho;
// 	RGB *imagemEntrada = par->imagemEntrada;
// 	RGB *imagemSaida = par->imagemSaida;
// 	RGB *imagemCinza = par->imagemCinza;
// 	RGB *imagemGaussiano = par->imagemGaussiano;
// 	int iForImagem, jForImagem;
// 	int iPosMatriz;
	
// 	for(iForImagem=id; iForImagem<cabecalho.altura; iForImagem+=nthr){
// 		for(jForImagem=0; jForImagem<cabecalho.largura; jForImagem++){
// 			iPosMatriz = iForImagem * cabecalho.largura + jForImagem;

// 			imagemCinza[iPosMatriz].red    = (0.2126 * imagemEntrada[iPosMatriz].red) + (0.7152 * imagemEntrada[iPosMatriz].green) + (0.0722 * imagemEntrada[iPosMatriz].blue);
// 			imagemCinza[iPosMatriz].green  = (0.2126 * imagemEntrada[iPosMatriz].red) + (0.7152 * imagemEntrada[iPosMatriz].green) + (0.0722 * imagemEntrada[iPosMatriz].blue);
// 			imagemCinza[iPosMatriz].blue   = (0.2126 * imagemEntrada[iPosMatriz].red) + (0.7152 * imagemEntrada[iPosMatriz].green) + (0.0722 * imagemEntrada[iPosMatriz].blue);
// 		}
// 	}
// }

int main(int argc, char **argv ){
	char *entrada;
	char *saida;
	char ali, aux;
	int  iForImagem, jForImagem;
	int  range;
	int  mascara;
	int  nthreads;
	int  i;
	double ti, tf;

	CABECALHO cabecalho;
	RGB *imagemEntrada;
	RGB *imagemSaida;
	RGB *imagemCinza;
	RGB *imagemGaussiano;
	pthread_t *tid = NULL;
	PARAMETROS *par = NULL;
	
	if ( argc != 5){
		printf("%s <img_entrada> <img_saida> <mascara> <threads>\n", argv[0]);
		exit(0);
	}

	entrada 	= argv[1];
	saida 		= argv[2];
	mascara		= atoi(argv[3]);
	nthreads	= atoi(argv[4]);

	FILE *fin = fopen(entrada, "rb");
	if ( fin == NULL ){
		printf("Erro ao abrir o arquivo entrada %s\n", entrada);
		exit(0);
	}

	FILE *fout = fopen(saida, "wb");
	if ( fout == NULL ){
		printf("Erro ao abrir o arquivo saida %s\n", saida);
		exit(0);
	}

	if ((mascara != 3) && (mascara != 5) && (mascara != 7)){
		printf("Erro, valor da mascara deve ser de 3,5,7\n");
		exit(0);
	}

	if (mascara == 3) {
		range = 1;
	}
	else if (mascara == 5) {
		range = 2;
	}
	else if (mascara == 7) {
		range = 3;
	}

	//Ler cabecalho entrada
	fread(&cabecalho, sizeof(CABECALHO), 1, fin);	

	//Alocar imagems
	imagemEntrada   = (RGB *)malloc(cabecalho.altura*cabecalho.largura*sizeof(RGB));
	imagemCinza  	= (RGB *)malloc(cabecalho.altura*cabecalho.largura*sizeof(RGB));
	imagemGaussiano = (RGB *)malloc(cabecalho.altura*cabecalho.largura*sizeof(RGB));
	imagemSaida  	= (RGB *)malloc(cabecalho.altura*cabecalho.largura*sizeof(RGB));

	//Leitura da imagem entrada
	for(iForImagem=0; iForImagem<cabecalho.altura; iForImagem++){
		ali = (cabecalho.largura * 3) % 4;

		if (ali != 0){
			ali = 4 - ali;
		}

		for(jForImagem=0; jForImagem<cabecalho.largura; jForImagem++){
			fread(&imagemEntrada[iForImagem * cabecalho.largura + jForImagem], sizeof(RGB), 1, fin);
		}

		for(jForImagem=0; jForImagem<ali; jForImagem++){
			fread(&aux, sizeof(unsigned char), 1, fin);
		}
	}

	ti = tempoCorrente();

	//Filtro em escala de cinza
	filtroCinza(imagemEntrada,imagemCinza,cabecalho);

	tid = (pthread_t *)malloc(nthreads * sizeof(pthread_t));
	par = (PARAMETROS *)malloc(nthreads * sizeof(PARAMETROS));

	for( i=0; i<nthreads; i++ ){
		par[i].id = i;
		par[i].nthr = nthreads;
		par[i].mascara = mascara;
		par[i].range = range;
		par[i].cabecalho = cabecalho;
		par[i].imagemEntrada = imagemEntrada;
		par[i].imagemSaida = imagemSaida;
		par[i].imagemCinza = imagemCinza;
		par[i].imagemGaussiano = imagemGaussiano;

		//Filtro Gaussiano
		pthread_create(&tid[i], NULL, filtroGaussiano, (void *) &par[i]); 
	} 
	
	for (i=0; i<nthreads; i++ ){
		pthread_join(tid[i], NULL);
	}

	//Aplicar filtro Sobel
	filtroSobel(imagemGaussiano, imagemSaida, cabecalho, range);
	
	//Escrever a imagem saida
	escreverImagem(imagemSaida,cabecalho,fout);

	tf = tempoCorrente();

	//Fecha os arquivos
	fclose(fin);
	fclose(fout);

	//Libera a memoria
	free(imagemEntrada);
	free(imagemCinza);
	free(imagemGaussiano);
	free(imagemSaida);
	free(tid);
	free(par);
	
	printf("Arquivo %s gerado com mascara de %d.\n", saida, mascara);
	printf("Tempo: %f\n", tf - ti );
}