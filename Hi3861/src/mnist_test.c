#include "../inc/mnist_test.h"
#include "../inc/nnom.h"
#include "../inc/mnist_densenet_weights.h"
#include "../inc/image.h"

nnom_model_t *model;

const char codeLib[] = "@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'.   ";
 
static void print_img(int8_t * buf)
{
    for(int y = 0; y < 28; y++) 
	{
        for (int x = 0; x < 28; x++) 
		{
            int index =  69 / 127.0 * (127 - buf[y*28+x]); 
			if(index > 69) index =69;
			if(index < 0) index = 0;
            printf("%c",codeLib[index]);
			printf("%c",codeLib[index]);
        }
        printf("\r\n");
    }
}

void model_test()
{
	model = nnom_model_create();    //创建模型
    model_run(model);               //运行模型
}

void mnist_pre(unsigned char index)
{
	uint32_t tick, time;
	uint32_t predic_label;	//预测出的标签
	float prob;
    
	int Probability;
 
	model_stat(model);
	printf("Total Memory cost (Network and NNoM): %d\r\n", nnom_mem_stat());
 
	memcpy(nnom_input_data, (int8_t*)&img[index][0], 784);
	nnom_predict(model, &predic_label, &prob);
 
	print_img((int8_t*)&img[index][0]);	
	printf("Truth label: %d\r\n", label[index]);
	printf("Predicted label: %d\r\n", predic_label);
	printf("Probability: %d%%\r\n", (int)(prob*100));
 
}