// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "VideoCaptureComponent.generated.h"

UCLASS( meta=(BlueprintSpawnableComponent) )
class EASYFFMPEG_API UVideoCaptureComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVideoCaptureComponent();
	
	UFUNCTION(BlueprintCallable, Category = "Video Capture")
	static void EncodeH264ToMP4(const FString& InH264FilePath, const FString& OutMp4FilePath);
	
};
