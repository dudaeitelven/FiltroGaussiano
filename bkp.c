#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>

#pragma pack(1)

/*
Para compilar:
1 - Abrir o local do fonte
2 - Digitar para compilar: gcc main.c -o main -lm
3 - Digitar para rodar: ./main borboleta.bmp saida.bmp 7 1
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

int main(int argc, char **argv ){
	char *entrada, *saida;
	char ali, aux;
	int mascara;
	int iForImagem, jForImagem;
	int range, meio;
	int lacoJ, limiteJ;
	int lacoI, limiteI;
	int iPosMatriz;
	int MascaraX[3*3], MascaraY[3*3];
	int matrizGaussiano[mascara*mascara];
	int valorX;
	int valorY;
	int iPosLinhaAnt, jPosColunaAnt;
	int iPosLinha, jPosColuna;
	int iPosLinhaPrx, jPosColunaPrx;
	int nthreads;
	int iForGaussiano, jForGaussiano;
	int iTamAux;

	CABECALHO cabecalho;
	RGB *imagemEntrada;
	RGB *imagemSaida;
	RGB *imagemCinza;
	RGB *imagemGaussiano;
	RGB *imagemAux;
	RGB *auxGaussiano;
	
	if ( argc != 5){
		printf("%s <img_entrada> <img_saida> <mascara> <threads>\n", argv[0]);
		exit(0);
	}

	entrada 	= argv[1];
	saida 		= argv[2];
    mascara		= atoi(argv[3]);
	nthreads	= atoi(argv[5]);
	
	if ((mascara != 3) && (mascara != 5) && (mascara != 7)){
		printf("Erro, valor da mascara deve ser de 3,5,7\n");
		exit(0);
	}

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

	//Ler cabecalho imagem entrada
	fread(&cabecalho, sizeof(CABECALHO), 1, fin);	

	//Alocar imagems
	imagemEntrada   = (RGB *)malloc(cabecalho.altura*cabecalho.largura*sizeof(RGB));
	imagemCinza  	= (RGB *)malloc(cabecalho.altura*cabecalho.largura*sizeof(RGB));
	imagemGaussiano = (RGB *)malloc(cabecalho.altura*cabecalho.largura*sizeof(RGB));
	imagemSaida  	= (RGB *)malloc(cabecalho.altura*cabecalho.largura*sizeof(RGB));
	imagemAux		= (RGB *)malloc(mascara*mascara*sizeof(RGB));
	auxGaussiano	= (RGB *)malloc(sizeof(RGB));

	if (mascara == 3) {
		range = 1;
	}
	else if (mascara == 5) {
		range = 2;
	}
	else if (mascara == 7) {
		range = 3;
	}

	//MascaraX - Filtro Sobel
	MascaraX[0] = -1; //P1
	MascaraX[1] =  0; //P2
	MascaraX[2] =  1; //P3
	MascaraX[3] = -2; //P4
	MascaraX[4] =  0; //P5 - Central
	MascaraX[5] =  2; //P6
	MascaraX[6] = -1; //P7
	MascaraX[7] =  0; //P8
	MascaraX[8] =  1; //P9

	//MascaraY - Filtro Sobel
	MascaraY[0] = -1; //P1
	MascaraY[1] = -2; //P2
	MascaraY[2] = -1; //P3
	MascaraY[3] =  0; //P4
	MascaraY[4] =  0; //P5 - Central
	MascaraY[5] =  0; //P6
	MascaraY[6] =  1; //P7
	MascaraY[7] =  2; //P8
	MascaraY[8] =  1; //P9

	//Mascara - Filtro Gaussiano
	if (mascara == 3) {
		matrizGaussiano[0] = 1;
		matrizGaussiano[1] = 2;
		matrizGaussiano[2] = 1;

		matrizGaussiano[3] = 2;
		matrizGaussiano[4] = 4;
		matrizGaussiano[5] = 2;

		matrizGaussiano[6] = 1;
		matrizGaussiano[7] = 2;
		matrizGaussiano[8] = 1;
	}
	else if (mascara == 5) {
		matrizGaussiano[0] = 1;
		matrizGaussiano[1] = 4;
		matrizGaussiano[2] = 7;
		matrizGaussiano[3] = 4;
		matrizGaussiano[4] = 1;

		matrizGaussiano[5] = 4;
		matrizGaussiano[6] = 16;
		matrizGaussiano[7] = 26;
		matrizGaussiano[8] = 16;
		matrizGaussiano[9] = 4;

		matrizGaussiano[10] = 7;
		matrizGaussiano[11] = 26;
		matrizGaussiano[12] = 41;
		matrizGaussiano[13] = 26;
		matrizGaussiano[14] = 7;

		matrizGaussiano[15] = 4;
		matrizGaussiano[16] = 16;
		matrizGaussiano[17] = 26;
		matrizGaussiano[18] = 16;
		matrizGaussiano[19] = 4;

		matrizGaussiano[20] = 1;
		matrizGaussiano[21] = 4;
		matrizGaussiano[22] = 7;
		matrizGaussiano[23] = 4;
		matrizGaussiano[24] = 1;
	}
	else {
		matrizGaussiano[0] = 0;
		matrizGaussiano[1] = 0;
		matrizGaussiano[2] = 1;
		matrizGaussiano[3] = 2;
		matrizGaussiano[4] = 1;
		matrizGaussiano[5] = 0;
		matrizGaussiano[6] = 0;

		matrizGaussiano[7]  =  0;
		matrizGaussiano[8]  =  3;
		matrizGaussiano[9]  = 13;
		matrizGaussiano[10] = 22;
		matrizGaussiano[11] = 13;
		matrizGaussiano[12] =  3;
		matrizGaussiano[13] =  0;

		matrizGaussiano[14] =  1;
		matrizGaussiano[15] = 13;
		matrizGaussiano[16] = 59;
		matrizGaussiano[17] = 97;
		matrizGaussiano[18] = 59;
		matrizGaussiano[19] = 13;
		matrizGaussiano[20] =  1;

		matrizGaussiano[21] =  2;
		matrizGaussiano[22] = 22;
		matrizGaussiano[23] = 97;
		matrizGaussiano[24] = 159;
		matrizGaussiano[25] = 97;
		matrizGaussiano[26] = 22;
		matrizGaussiano[27] =  2;

		matrizGaussiano[28] =  1;
		matrizGaussiano[29] = 13;
		matrizGaussiano[30] = 59;
		matrizGaussiano[31] = 97;
		matrizGaussiano[32] = 59;
		matrizGaussiano[33] = 13;
		matrizGaussiano[34] =  1;

		matrizGaussiano[35] =  0;
		matrizGaussiano[36] =  3;
		matrizGaussiano[37] = 13;
		matrizGaussiano[38] = 22;
		matrizGaussiano[39] = 13;
		matrizGaussiano[40] =  3;
		matrizGaussiano[41] =  0;

		matrizGaussiano[42] = 0;
		matrizGaussiano[43] = 0;
		matrizGaussiano[44] = 1;
		matrizGaussiano[45] = 2;
		matrizGaussiano[46] = 1;
		matrizGaussiano[47] = 0;
		matrizGaussiano[48] = 0;
	}

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

	//Transformar em escala de cinza
	for(iForImagem=0; iForImagem<cabecalho.altura; iForImagem++){
		for(jForImagem=0; jForImagem<cabecalho.largura; jForImagem++){
			iPosMatriz = iForImagem * cabecalho.largura + jForImagem;

			imagemCinza[iPosMatriz].red    = (0.2126 * imagemEntrada[iPosMatriz].red) + (0.7152 * imagemEntrada[iPosMatriz].green) + (0.0722 * imagemEntrada[iPosMatriz].blue);
			imagemCinza[iPosMatriz].green  = (0.2126 * imagemEntrada[iPosMatriz].red) + (0.7152 * imagemEntrada[iPosMatriz].green) + (0.0722 * imagemEntrada[iPosMatriz].blue);
			imagemCinza[iPosMatriz].blue   = (0.2126 * imagemEntrada[iPosMatriz].red) + (0.7152 * imagemEntrada[iPosMatriz].green) + (0.0722 * imagemEntrada[iPosMatriz].blue);
		}
	}

	//Aplicar filtro Gaussiano
	for(iForImagem=1; iForImagem<(cabecalho.altura-1); iForImagem++){
		for(jForImagem=1; jForImagem<(cabecalho.largura-1); jForImagem++){
			//Limpar auxiliares
			for(iTamAux=0; iTamAux<mascara*mascara; iTamAux++){
                imagemAux[iTamAux].red   = 0;
                imagemAux[iTamAux].green = 0;
                imagemAux[iTamAux].blue  = 0;
			}

			iTamAux = 0;

			//Buscar valores da imagem em cinza
			for (iForGaussiano = (iForImagem - range); iForGaussiano < (iForImagem + range); iForGaussiano++)
            {
                for (jForGaussiano = (jForGaussiano - range); jForGaussiano < (jForGaussiano + range); jForGaussiano++)
                {
					iPosMatriz = iForImagem * cabecalho.largura + jForImagem;

					imagemAux[iTamAux].red   = imagemCinza[iPosMatriz].red;
					imagemAux[iTamAux].green = imagemCinza[iPosMatriz].green;
					imagemAux[iTamAux].blue  = imagemCinza[iPosMatriz].blue;

					iTamAux++;
                }
            }

			//Calcular o gaussiano
			for (iForGaussiano = 0; iForGaussiano < range; iForGaussiano++)
            {
                for (jForGaussiano = 0; jForGaussiano < range; jForGaussiano++)
                {	
					iPosMatriz = (iForGaussiano * mascara) + jForGaussiano;

					auxGaussiano[0].red 	= auxGaussiano[0].red   + (imagemAux[iPosMatriz].red   * imagemCinza[iPosMatriz].red);
					auxGaussiano[0].green 	= auxGaussiano[0].green + (imagemAux[iPosMatriz].green * imagemCinza[iPosMatriz].green);
					auxGaussiano[0].blue 	= auxGaussiano[0].blue  + (imagemAux[iPosMatriz].blue  * imagemCinza[iPosMatriz].blue);
                }
            }

            //Substituir valores de cada pixel
			iPosMatriz = iForImagem * cabecalho.largura + jForImagem;

			imagemGaussiano[iPosMatriz].red    = auxGaussiano[0].red   / (mascara*mascara);
			imagemGaussiano[iPosMatriz].green  = auxGaussiano[0].green / (mascara*mascara);
			imagemGaussiano[iPosMatriz].blue   = auxGaussiano[0].blue  / (mascara*mascara);
		}
	}

	//Aplicar filtro Sobel
	for(iForImagem=1; iForImagem<(cabecalho.altura-1); iForImagem++){
		for(jForImagem=1; jForImagem<(cabecalho.largura-1); jForImagem++){
			//Calcular as posicoes
			iPosLinhaAnt  = (iForImagem-1) * cabecalho.largura;
			jPosColunaAnt = jForImagem-1;

			iPosLinha  = (iForImagem) * cabecalho.largura;
			jPosColuna = jForImagem;

			iPosLinhaPrx  = (iForImagem+1) * cabecalho.largura;
			jPosColunaPrx = jForImagem+1;

			//Mascaras
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
	
	fclose(fin);
	fclose(fout);
	
	printf("Arquivo %s gerado.\n", saida);
}