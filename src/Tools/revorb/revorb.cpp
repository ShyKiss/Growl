#include "revorb.h"
#include <WinUser.h>
#include <filesystem>

bool Failed = false;

bool CopyHeaders(FILE *fi, ogg_sync_state *si, ogg_stream_state *is, FILE *fo, ogg_sync_state *so, ogg_stream_state *os, vorbis_info *vi)
{
    char *buffer = ogg_sync_buffer(si, 4096);
    int numread = fread(buffer, 1, 4096, fi);
    ogg_sync_wrote(si, numread);

    ogg_page page;

    if (ogg_sync_pageout(si, &page) != 1)
    {
        fprintf(stderr, "Input is not an Ogg.\n");
        return false;
    }

    ogg_stream_init(is, ogg_page_serialno(&page));
    ogg_stream_init(os, ogg_page_serialno(&page));

    if (ogg_stream_pagein(is,&page) < 0)
    {
        fprintf(stderr, "Error in the first page.\n");
        ogg_stream_clear(is);
        ogg_stream_clear(os);
        return false;
    }

    ogg_packet packet;

    if (ogg_stream_packetout(is,&packet) != 1)
    {
        fprintf(stderr, "Error in the first packet.\n");
        ogg_stream_clear(is);
        ogg_stream_clear(os);
        return false;
    }

    vorbis_comment vc;
    vorbis_comment_init(&vc);

    if (vorbis_synthesis_headerin(vi, &vc, &packet) < 0)
    {
        fprintf(stderr, "Error in header, probably not a Vorbis file.\n");
        vorbis_comment_clear(&vc);
        ogg_stream_clear(is);
        ogg_stream_clear(os);
        return false;
    }

    ogg_stream_packetin(os, &packet);

    int i = 0;

    while (i < 2)
    {
        int res = ogg_sync_pageout(si, &page);

        if (res == 0)
        {
            buffer = ogg_sync_buffer(si, 4096);
            numread = fread(buffer, 1, 4096, fi);

            if (numread == 0 && i < 2)
            {
                fprintf(stderr, "Headers are damaged, file is probably truncated.\n");
                ogg_stream_clear(is);
                ogg_stream_clear(os);
                return false;
            }

            ogg_sync_wrote(si, 4096);
            continue;
        }

        if (res == 1)
        {
            ogg_stream_pagein(is, &page);

            while (i < 2)
            {
                res = ogg_stream_packetout(is, &packet);

                if (res == 0)
                {
                    break;
                }
                else if (res < 0)
                {
                    fprintf(stderr, "Secondary header is corrupted.\n");
                    vorbis_comment_clear(&vc);
                    ogg_stream_clear(is);
                    ogg_stream_clear(os);
                    return false;
                }

                vorbis_synthesis_headerin(vi, &vc, &packet);
                ogg_stream_packetin(os, &packet);
                i++;
            }
        }
    }

    vorbis_comment_clear(&vc);

    while (ogg_stream_flush(os,&page))
    {
        if (fwrite(page.header, 1, page.header_len, fo) != page.header_len || fwrite(page.body, 1, page.body_len, fo) != page.body_len)
        {
            fprintf(stderr,"Cannot write headers to output.\n");
            ogg_stream_clear(is);
            ogg_stream_clear(os);
            return false;
        }
    }

    return true;
}

/*int32_t revorb(const std::string& oggpath)
{

    FILE* fi;
    FILE* fo;
    //char tmpName[256];
    std::string tmpName = oggpath + ".tmp";

    fopen_s(&fi, oggpath.c_str(), "rb");
    //strcat_s(tmpName, sizeof(tmpName), oggpath.c_str());
    //strcat_s(tmpName, sizeof(tmpName), ".tmp");
    fopen_s(&fo, tmpName.c_str(), "wb");

    ogg_sync_state sync_in, sync_out;
    ogg_sync_init(&sync_in);
    ogg_sync_init(&sync_out);

    ogg_stream_state stream_in, stream_out;
    vorbis_info vi;
    vorbis_info_init(&vi);

    ogg_packet packet;
    ogg_page page;

    if (CopyHeaders(fi, &sync_in, &stream_in, fo, &sync_out, &stream_out, &vi))
    {
        ogg_int64_t granpos = 0, packetnum = 0;
        int lastbs = 0;

        while (true)
        {
            int eos = 0;

            while (!eos)
            {
                int res = ogg_sync_pageout(&sync_in, &page);

                if (res == 0)
                {
                    char* buffer = ogg_sync_buffer(&sync_in, 4096);
                    int numread = fread(buffer, 1, 4096, fi);

                    if (numread > 0)
                    {
                        ogg_sync_wrote(&sync_in, numread);
                    }
                    else
                    {
                        eos = 2;
                    }

                    continue;
                }

                if (res < 0)
                {
                    fprintf(stderr, "Warning: Corrupted or missing data in bitstream.\n");
                    Failed = true;
                }
                else
                {
                    if (ogg_page_eos(&page))
                    {
                        eos = 1;
                    }

                    ogg_stream_pagein(&stream_in, &page);

                    while (true)
                    {
                        res = ogg_stream_packetout(&stream_in, &packet);

                        if (res == 0)
                        {
                            break;
                        }
                        else if (res < 0)
                        {
                            fprintf(stderr, "Warning: Bitstream error.\n");
                            Failed = true;
                            continue;
                        }

                        int bs = vorbis_packet_blocksize(&vi, &packet);

                        if (lastbs)
                        {
                            granpos += (lastbs + bs) / 4;
                        }

                        lastbs = bs;
                        packet.granulepos = granpos;
                        packet.packetno = packetnum++;

                        if (!packet.e_o_s)
                        {
                            ogg_stream_packetin(&stream_out, &packet);

                            ogg_page opage;

                            while (ogg_stream_pageout(&stream_out, &opage))
                            {
                                if (fwrite(opage.header, 1, opage.header_len, fo) != opage.header_len || fwrite(opage.body, 1, opage.body_len, fo) != opage.body_len)
                                {
                                    fprintf(stderr, "Unable to write page to output.\n");
                                    eos = 2;
                                    Failed = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            if (eos == 2)
            {
                break;
            }

            packet.e_o_s = 1;
            ogg_stream_packetin(&stream_out, &packet);
            ogg_page opage;

            while (ogg_stream_flush(&stream_out, &opage))
            {
                if (fwrite(opage.header, 1, opage.header_len, fo) != opage.header_len || fwrite(opage.body, 1, opage.body_len, fo) != opage.body_len)
                {
                    fprintf(stderr, "Unable to write page to output.\n");
                    Failed = true;
                    break;
                }
            }

            ogg_stream_clear(&stream_in);
            break;
        }

        ogg_stream_clear(&stream_out);
    }
    else
    {
        Failed = true;
    }

    vorbis_info_clear(&vi);

    ogg_sync_clear(&sync_in);
    ogg_sync_clear(&sync_out);

    fclose(fi);
    fclose(fo);

    //if (Failed)
    //{
    //    //_unlink(tmpName.c_str());
    //    //strcat_s(tmpName, sizeof(tmpName), ".tmp");
    //}
    //else
    //{
        //if (_unlink(oggpath.c_str()) || rename(tmpName, oggpath.c_str()))
        //{
        //    fprintf(stderr, "%S: Could not put the output file back in place.\n", tmpName);
        //}
    //}

    return EXIT_SUCCESS;
}*/
std::string revorb(std::string oggpath)
{
    FILE* fi;
    
    fopen_s(&fi, oggpath.c_str(), "rb");

    if (!fi)
    {
        fprintf(stderr, "Could not open input file.\n");
        //return 22;
    }

    //char tmpName{}[MAX_PATH];
    FILE* fo;
    std::filesystem::path p(oggpath);
    std::string tmpName = p.replace_extension(".tmp").string();

    //strcat_s(tmpName.c_str(), sizeof(tmpName.c_str()), oggpath.c_str());
    //strcat_s(tmpName, sizeof(tmpName), ".tmp");

    fopen_s(&fo, tmpName.c_str(), "wb");

    if (!fo)
    {
        fprintf(stderr, "Could not open output file.\n");
        fclose(fi);
        //return 44;
    }

    //Failed = false;

    ogg_sync_state sync_in, sync_out;
    ogg_sync_init(&sync_in);
    ogg_sync_init(&sync_out);

    ogg_stream_state stream_in, stream_out;
    vorbis_info vi;
    vorbis_info_init(&vi);

    ogg_packet packet;
    ogg_page page;

    if (CopyHeaders(fi, &sync_in, &stream_in, fo, &sync_out, &stream_out, &vi))
    {
        ogg_int64_t granpos = 0, packetnum = 0;
        int lastbs = 0;

        while (true)
        {
            int eos = 0;

            while (!eos)
            {
                int res = ogg_sync_pageout(&sync_in, &page);

                if (res == 0)
                {
                    char* buffer = ogg_sync_buffer(&sync_in, 4096);
                    int numread = fread(buffer, 1, 4096, fi);

                    if (numread > 0)
                    {
                        ogg_sync_wrote(&sync_in, numread);
                    }
                    else
                    {
                        eos = 2;
                    }

                    continue;
                }

                if (res < 0)
                {
                    fprintf(stderr, "Warning: Corrupted or missing data in bitstream.\n");
                    Failed = true;
                }
                else
                {
                    if (ogg_page_eos(&page))
                    {
                        eos = 1;
                    }

                    ogg_stream_pagein(&stream_in, &page);

                    while (true)
                    {
                        res = ogg_stream_packetout(&stream_in, &packet);

                        if (res == 0)
                        {
                            break;
                        }
                        else if (res < 0)
                        {
                            fprintf(stderr, "Warning: Bitstream error.\n");
                            Failed = true;
                            continue;
                        }

                        int bs = vorbis_packet_blocksize(&vi, &packet);

                        if (lastbs)
                        {
                            granpos += (lastbs + bs) / 4;
                        }

                        lastbs = bs;
                        packet.granulepos = granpos;
                        packet.packetno = packetnum++;

                        if (!packet.e_o_s)
                        {
                            ogg_stream_packetin(&stream_out, &packet);

                            ogg_page opage;

                            while (ogg_stream_pageout(&stream_out, &opage))
                            {
                                if (fwrite(opage.header, 1, opage.header_len, fo) != opage.header_len || fwrite(opage.body, 1, opage.body_len, fo) != opage.body_len)
                                {
                                    fprintf(stderr, "Unable to write page to output.\n");
                                    eos = 2;
                                    Failed = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            if (eos == 2)
            {
                break;
            }

            packet.e_o_s = 1;
            ogg_stream_packetin(&stream_out, &packet);
            ogg_page opage;

            while (ogg_stream_flush(&stream_out, &opage))
            {
                if (fwrite(opage.header, 1, opage.header_len, fo) != opage.header_len || fwrite(opage.body, 1, opage.body_len, fo) != opage.body_len)
                {
                    fprintf(stderr, "Unable to write page to output.\n");
                    Failed = true;
                    break;
                }
            }

            ogg_stream_clear(&stream_in);
            break;
        }

        ogg_stream_clear(&stream_out);
    }
    else
    {
        Failed = true;
    }

    vorbis_info_clear(&vi);

    ogg_sync_clear(&sync_in);
    ogg_sync_clear(&sync_out);

    fclose(fi);
    fclose(fo);

    if (true)
    {
        if (Failed)
        {
            //_unlink(tmpName);
            //strcat_s(tmpName, sizeof(tmpName), ".tmp");
        }
        else
        {
            if (_unlink(oggpath.c_str()) || rename(tmpName.c_str(), oggpath.c_str()))
            {
                //fprintf(stderr, "%S: Could not put the output file back in place.\n", tmpName.c_str());
            }
        }
    }

    return oggpath;
}