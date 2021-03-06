#include "meta.h"
#include "../util.h"

/* .dsp found in:
    Hikaru No Go 3 (NGC)
    Yu-Gi-Oh! The Falsebound Kingdom (NGC)

    2010-01-31 - added loop stuff and some header checks...
*/

VGMSTREAM * init_vgmstream_dsp_ygo(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[PATH_LIMIT];
    int loop_flag;
    int channel_count;
    off_t start_offset;
    int i;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("dsp",filename_extension(filename))) goto fail;

    /* check file size with size given in header */
    if ((read_32bitBE(0x0,streamFile)+0xE0) != (get_streamfile_size(streamFile)))
        goto fail;

    loop_flag = (uint16_t)(read_16bitBE(0x2C,streamFile) != 0x0);
    channel_count = 1;
    
    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    /* fill in the vital statistics */
    start_offset = 0xE0;
    vgmstream->channels = channel_count;
    vgmstream->sample_rate = read_32bitBE(0x28,streamFile);
    vgmstream->coding_type = coding_NGC_DSP;
    vgmstream->num_samples = read_32bitBE(0x20,streamFile);
    vgmstream->layout_type = layout_none;
    vgmstream->meta_type = meta_DSP_YGO;
    if (loop_flag) {
        vgmstream->loop_start_sample = (read_32bitBE(0x30,streamFile)*14/16);
        vgmstream->loop_end_sample = (read_32bitBE(0x34,streamFile)*14/16);
    }

    // read coef stuff
    {
        for (i=0;i<16;i++) {
            vgmstream->ch[0].adpcm_coef[i] = read_16bitBE(0x3C+i*2,streamFile);
        }
    }

    /* open the file for reading */
    {
        int i;
        STREAMFILE * file;
        file = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!file) goto fail;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = file;
            vgmstream->ch[i].channel_start_offset=
                vgmstream->ch[i].offset=start_offset+
                vgmstream->interleave_block_size*i;
        }
    }

    return vgmstream;

fail:
    /* clean up anything we may have opened */
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
