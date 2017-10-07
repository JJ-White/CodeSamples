#define DEAD 0;
#define ALIVE 1;

__kernel void gameOfLife(
	__global int* gridA,
	__global int* gridB,
	__write_only image2d_t image)
{
	int width = get_global_size(0);
	int height = get_global_size(1);
	int posX = get_global_id(0);
	int posY = get_global_id(1);
	
	//Get surrounding pixels
	int pos = posY * width + posX;
	int neighbors = 0;
	int mWidth = width - 1;
	int mHeight = height - 1;
	
	if (posX > 0 && posY > 0 && gridA[pos - width - 1]) neighbors++;
	if (posY > 0 && gridA[pos - width]) neighbors++;
	if (posX < mWidth && posY > 0 && gridA[pos - width + 1]) neighbors++;
	if (posX > 0 && gridA[pos - 1]) neighbors++;
	if (posX < mWidth && gridA[pos + 1]) neighbors++;
	if (posX > 0 && posY < mHeight && gridA[pos + width - 1]) neighbors++;
	if (posY < mHeight && gridA[pos + width]) neighbors++;
	if (posX < mWidth && posY < mHeight && gridA[pos + width + 1]) neighbors++;

	//Determine fate
	int fate = gridA[pos];
	if ((fate && (neighbors == 2 || neighbors == 3)) || (!fate && neighbors == 3)) {
		fate = ALIVE;
	}
	else {
		fate = DEAD;
	}

	//Write result to output grid
	gridB[pos] = fate;

	//Add pixel to output image
	int2 pixel = (int2)(posX, mHeight - posY);
	float4 dead = (float4)(0.0, 0.0, 0.0, 1.0);
	float4 alive = (float4)(1.0, 1.0, 1.0, 1.0);
	write_imagef(image, pixel, fate ? alive : dead);
}

__kernel void gameOfLifeB(
	__global int* gridA,
	__global int* gridB,
	__write_only image2d_t image)
{
	int width = get_global_size(0) + 2;
	int height = get_global_size(1) + 2;
	int posX = get_global_id(0) + 1;
	int posY = get_global_id(1) + 1;

	//Get surrounding pixels
	int pos = posY * width + posX;
	int neighbors = 0;
	int mHeight = height - 1;

	if (gridA[pos - width - 1]) neighbors++;
	if (gridA[pos - width]) neighbors++;
	if (gridA[pos - width + 1]) neighbors++;
	if (gridA[pos - 1]) neighbors++;
	if (gridA[pos + 1]) neighbors++;
	if (gridA[pos + width - 1]) neighbors++;
	if (gridA[pos + width]) neighbors++;
	if (gridA[pos + width + 1]) neighbors++;

	//Determine fate
	int fate = gridA[pos];
	if ((fate && (neighbors == 2 || neighbors == 3)) || (!fate && neighbors == 3)) {
		fate = ALIVE;
	}
	else {
		fate = DEAD;
	}

	//Write result to output grid
	gridB[pos] = fate;

	//Add pixel to output image
	int2 pixel = (int2)(posX - 1, mHeight - posY - 1);
	float4 dead = (float4)(0.0, 0.0, 0.0, 1.0);
	float4 alive = (float4)(1.0, 1.0, 1.0, 1.0);
	write_imagef(image, pixel, fate ? alive : dead);
}

__kernel void gameOfLifeC(
	__global int* gridA,
	__global int* gridB,
	__write_only image2d_t image)
{
	int width = get_global_size(0) + 2;
	int height = get_global_size(1) + 2;
	int posX = get_global_id(0) + 1;
	int posY = get_global_id(1) + 1;

	//Get surrounding pixels
	int pos = posY * width + posX;
	int neighbors = 0;

	int localGrid[] = {
		gridA[pos - width - 1],
		gridA[pos - width],
		gridA[pos - width + 1],
		gridA[pos - 1],
		gridA[pos + 1],
		gridA[pos + width - 1],
		gridA[pos + width],
		gridA[pos + width + 1]
	};

	for (int i = 0; i < 8; i++) {
		if (localGrid[i]) neighbors++;
	}

	//Determine fate
	int fate = gridA[pos];
	if ((fate && (neighbors == 2 || neighbors == 3)) || (!fate && neighbors == 3)) {
		fate = ALIVE;
	}
	else {
		fate = DEAD;
	}

	//Write result to output grid
	gridB[pos] = fate;

	//Add pixel to output image
	int mHeight = height - 1;
	int2 pixel = (int2)(posX - 1, mHeight - posY - 1);
	float4 dead = (float4)(0.0, 0.0, 0.0, 1.0);
	float4 alive = (float4)(1.0, 1.0, 1.0, 1.0);
	write_imagef(image, pixel, fate ? alive : dead);
}
