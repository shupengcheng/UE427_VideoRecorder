// Fill out your copyright notice in the Description page of Project Settings.


#include "VideoCaptureComponent.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
#include "libavformat/avformat.h"
}

// Sets default values for this component's properties
UVideoCaptureComponent::UVideoCaptureComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	// ...
}

void UVideoCaptureComponent::EncodeH264ToMP4(const FString& InH264FilePath, const FString& OutMp4FilePath)
{
    // 初始化 FFmpeg 库
    av_register_all();

    // 打开输入 H.264 文件
    AVFormatContext* InFmtCtx = nullptr;
    if (avformat_open_input(&InFmtCtx, TCHAR_TO_ANSI(*InH264FilePath), nullptr, nullptr) != 0)
    {
        UE_LOG(LogTemp, Error, TEXT("无法打开输入文件."));
        return;
    }

    // 获取输入流信息
    if (avformat_find_stream_info(InFmtCtx, nullptr) < 0)
    {
        UE_LOG(LogTemp, Error, TEXT("无法找到流信息."));
        avformat_close_input(&InFmtCtx);
        return;
    }

    // 创建输出文件上下文
    AVFormatContext* OutFmtCtx = nullptr;
    avformat_alloc_output_context2(&OutFmtCtx, nullptr, nullptr, TCHAR_TO_ANSI(*OutMp4FilePath));
    if (!OutFmtCtx)
    {
        UE_LOG(LogTemp, Error, TEXT("无法创建输出文件上下文."));
        avformat_close_input(&InFmtCtx);
        return;
    }

    // 获取输入流（假设只有一个视频流）
    AVStream* InStream = InFmtCtx->streams[0];
    // 在输出文件中创建一个新的视频流
    AVStream* OutStream = avformat_new_stream(OutFmtCtx, nullptr);
    if (!OutStream)
    {
        UE_LOG(LogTemp, Error, TEXT("无法创建输出流."));
        avformat_close_input(&InFmtCtx);
        avformat_free_context(OutFmtCtx);
        return;
    }

    // 复制输入流的编码器参数到输出流
    if (avcodec_parameters_copy(OutStream->codecpar, InStream->codecpar) < 0)
    {
        UE_LOG(LogTemp, Error, TEXT("无法复制编解码器参数."));
        avformat_close_input(&InFmtCtx);
        avformat_free_context(OutFmtCtx);
        return;
    }

    // 设置时间基
    OutStream->time_base = {1, 30}; // 假设帧率为 30 fps，适当调整以匹配实际情况

    // 打开输出文件
    if (!(OutFmtCtx->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_open(&OutFmtCtx->pb, TCHAR_TO_ANSI(*OutMp4FilePath), AVIO_FLAG_WRITE) < 0)
        {
            UE_LOG(LogTemp, Error, TEXT("无法打开输出文件."));
            avformat_close_input(&InFmtCtx);
            avformat_free_context(OutFmtCtx);
            return;
        }
    }

    // 写入文件头信息
    if (avformat_write_header(OutFmtCtx, nullptr) < 0)
    {
        UE_LOG(LogTemp, Error, TEXT("写入文件头时出错."));
        avio_closep(&OutFmtCtx->pb);
        avformat_close_input(&InFmtCtx);
        avformat_free_context(OutFmtCtx);
        return;
    }

    AVPacket Packet1;
    int64_t LastDts = 0;
    int64_t LastPts = 0;

    // 读取每一个视频帧并写入输出文件
    while (av_read_frame(InFmtCtx, &Packet1) >= 0)
    {
        // 将包的流索引设置为输出流的索引
        Packet1.stream_index = OutStream->index;

        // 重新设置包的时间戳
        if (Packet1.pts != AV_NOPTS_VALUE)
        {
            Packet1.pts = av_rescale_q(Packet1.pts, InStream->time_base, OutStream->time_base);
            LastPts = Packet1.pts;
        }
        else
        {
            Packet1.pts = LastPts + av_rescale_q(1, OutStream->time_base, InStream->time_base);
            LastPts = Packet1.pts;
        }

        if (Packet1.dts != AV_NOPTS_VALUE)
        {
            Packet1.dts = av_rescale_q(Packet1.dts, InStream->time_base, OutStream->time_base);
            LastDts = Packet1.dts;
        }
        else
        {
            Packet1.dts = LastDts + av_rescale_q(1, OutStream->time_base, InStream->time_base);
            LastDts = Packet1.dts;
        }

        Packet1.duration = av_rescale_q(Packet1.duration, InStream->time_base, OutStream->time_base);
        Packet1.pos = -1;

        // 写入包到输出文件
        if (av_interleaved_write_frame(OutFmtCtx, &Packet1) < 0)
        {
            UE_LOG(LogTemp, Error, TEXT("封装包时出错."));
            av_packet_unref(&Packet1);
            break;
        }

        av_packet_unref(&Packet1);
    }

    // 写入文件尾信息
    av_write_trailer(OutFmtCtx);

    // 关闭输入和输出文件
    avformat_close_input(&InFmtCtx);

    if (OutFmtCtx && !(OutFmtCtx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&OutFmtCtx->pb);
    avformat_free_context(OutFmtCtx);

    UE_LOG(LogTemp, Log, TEXT("成功将 H.264 封装为 MP4 文件."));
}
