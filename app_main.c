//EU//
// Bibliotecas//
#include <stdio.h> //---> Biblioteca de Entrada e Saida Padrao do C (Permite Algumas funcoes como printf)//
#include "freertos/FreeRTOS.h" //---> Sistema Operacional do ESP32//
#include "esp_vfs_fat.h"              //-------------------> Bibliotecas para Permitir Salvar Fotos em um Cartao SD//
#include "sdmmc_cmd.h"//--------------|
#include "freertos/task.h" //---> Biblioteca para Gerenciamento de Tarefas do Sistema Operacional do ESP32//
#include "esp_camera.h" //---> Biblioteca da Camera do ESPCAM//
#include "esp_camera_af.h" //---> Biblioteca para Foco Automatico do ESPCAM//
#include "driver/gpio.h" //----> Biblioteca para Configuracao dos Pinos//

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define FLASH_PIN          4
 
//Função para Configurar o Botão, configurando o pino de entrada que no caso seria -1//
void configurar_botao()
{
    gpio_reset_pin(-1);
    gpio_set_direction(1, GPIO_MODE_INPUT);
}

//I.A//
//Variavel para Armazenar a Foto//
camera_fb_t *foto = NULL; 
/*Configurações da Camera do ESPCAM:
Abaixo são definidos:
                       - Os pinos de conexão (PWDN, RESET, XCLK, SIOD, SIOC, Y9, Y8, Y7, Y6, Y5, Y4, Y3, Y2, VSYNC, HREF e PCLK)
                       - A frequência do XCLK 
                       (Frequência do XCLK seria o clock na camera que nada mais e que o flash da câmera, ou seja, e a frequência que a câmera utiliza para processar as imagens)
                       - O formato dos pixels (PIXFORMAT_JPEG, mostra que as imagens serao capturadas no formato JPEG),
                       - A qualidade da imagem (.frame_size = FRAMESIZE_VGA, define o tamanho da imagem que vai ser 640x480 pixels)*/
    camera_config_t config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_VGA,
    .jpeg_quality = 12,
    .fb_count = 1
};

//EU//
//O typedef enum vai declarar os nomes/valores dos estados, para evitar repetição de linhas como as linhas 73 a 79//
typedef enum { 
    INICIALIZAR_CAMERA,
    AGUARDANDO_COMANDO,
    CAPTURANDO_FOTO,
    UPLOAD,
    ERRO
} Estado;

void app_main()  
{   //Configurar o botão para realizar a foto//
    configurar_botao();
    printf("Aguardando Comando...\n");
    Estado estado = INICIALIZAR_CAMERA;

/*Dentro de um laço while (Tudo que estiver dentro do while vai se repetir) 
  Tem um switch case que é usado para simplificar/encurtar o codigo substituindo ter varias linhas contendo if ou else, o switch verifica a variavel que colocamos e executa o código da variavel*/
while (1)
{
switch(estado)
{
  case INICIALIZAR_CAMERA:
{
    printf("Inicializando camera...\n");
    esp_err_t err = esp_camera_init(&config);
    if(err == ESP_OK)
    {
        printf("Camera OK\n");
        if(iniciar_sd() == ESP_OK)
        {
            printf("SD OK\n");
            estado = AGUARDANDO_COMANDO;
        }
        else
        {
            printf("Erro SD\n");
            estado = ERRO;
        }
    }
    else
    {
        printf("Erro camera\n");
        estado = ERRO;
    }

    break;
}

    case AGUARDANDO_COMANDO:
//
    if(gpio_get_level(13) == 0)
    {
        estado = CAPTURANDO_FOTO;
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    break;

    case CAPTURANDO_FOTO:
{
    foto = esp_camera_fb_get();

    if(foto == NULL)
    {
        printf("Falha na captura\n");
        estado = ERRO;
    }
    else
    {
        printf("Foto capturada\n");
        printf("Tamanho: %u bytes\n", (unsigned int)foto->len);

        estado = UPLOAD;
    }

    break;
}
    break;

    case UPLOAD:
{
    //I.A//
    /*Configuracoes para salvar uma foto em arquivo no cartão microSD
    - Na linha 165, a I.A criou um programa tenta criar o arquivo foto.jpg no cartão, se já existir o arquivo será apagado e um novo será criado.
    - Nas linhas 166. a 168, é para verificar se deu erro, por exemplo se o arquivo for NULL é porque não foi possivel criar o arquivo, nesse caso aparece a mensagem de erro ("Erro ao criar arquivo\n")
    - Nas linhas 170 a 173, Libera a memória usada pela foto, logo após ele informa ao programa que ocorreu um erro, se conseguiu criar o arquivo significa que o arquivo foi criado corretamente.*/
    FILE *arquivo = fopen("/sdcard/foto.jpg", "wb");
    if(arquivo == NULL)
    {
        printf("Erro ao criar arquivo\n");

        esp_camera_fb_return(foto);

        estado = ERRO;
    }
    else
    {
        /* - Na linha 175, os dados da foto vai ser copiado da memória para o arquivo no cartão microSD
           - Na linha 176, termina o save da fto (exemplo: seria como clicar em "salvar" e fechar o arquivo)
           - Na linha 177, ele avisa que deu certo e mostra a mensagem ("Foto salva no SD\n" )*/
        fwrite(foto->buf, 1, foto->len, arquivo);
        fclose(arquivo);
        printf("Foto salva no SD\n");
        esp_camera_fb_return(foto);

        foto = NULL;

        estado = AGUARDANDO_COMANDO;
    }

    break;
}

    case ERRO:
    printf("Falha no processo\n");
    vTaskDelay(pdMS_TO_TICKS(2000));

    estado = AGUARDANDO_COMANDO;
    break;
}
}
}