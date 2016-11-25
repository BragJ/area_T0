/*
 * firewiretool.cpp
 *
 *  Created on: Mar 20, 2010
 *      Author: ulrik
 */

#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>

#include <ellLib.h>


#include <dc1394/dc1394.h>
/** Number of image buffers the dc1394 library will use internally */
#define FDC_DC1394_NUM_BUFFERS 5
/** Print an errorcode to stderr. */
#define ERR(errCode) \
    do { \
        int __err__ = (errCode); \
        if (__err__ != 0) \
            fprintf(stderr, \
                "ERROR [%s:%d]: dc1394 code: %d\n", \
                __FILE__, __LINE__, __err__); \
    } while (0)

typedef struct {
    dc1394color_coding_t code;
    const char * colorCodeStr;
} dc1394_colorCodeString_t;
static dc1394_colorCodeString_t colorCodeString_lut[] = {
    {DC1394_COLOR_CODING_MONO8,   "MONO8"},
    {DC1394_COLOR_CODING_YUV411,  "YUV411"},
    {DC1394_COLOR_CODING_YUV422,  "YUV422"},
    {DC1394_COLOR_CODING_YUV444,  "YUV444"},
    {DC1394_COLOR_CODING_RGB8,    "RGB8"},
    {DC1394_COLOR_CODING_MONO16,  "MONO16"},
    {DC1394_COLOR_CODING_RGB16,   "RGB16"},
    {DC1394_COLOR_CODING_MONO16S, "MONO16S"},
    {DC1394_COLOR_CODING_RGB16S,  "RGB16S"},
    {DC1394_COLOR_CODING_RAW8,    "RAW8"},
    {DC1394_COLOR_CODING_RAW16,   "RAW16"},
};

struct camLinkedList {
    camLinkedList *next;
    dc1394camera_t *camera;
};
typedef struct camLinkedList camListItem_t;

static struct option longopts[] = {
        {"verbose",         no_argument,        NULL,   0},
        {"camid",           required_argument,  NULL,   1},
        {"reset",           no_argument,        NULL,   2},
        {"report",          optional_argument,  NULL,   3},
        {"help",            no_argument,        NULL,   4},
        { NULL,             0,                  NULL,   0}
};

struct fwtooloptions_t {
    int verbose;
    int reset;
    int report;
    char *camid;
};
static struct fwtooloptions_t fwtopts = { 0, 0, 0, NULL };
void parseOptions(int argc, char* argv[]);

dc1394camera_t* findcam(dc1394camera_t **cams, int ncams, char* guid);
int allocate_cams(dc1394_t *bus, dc1394camera_list_t *list, dc1394camera_t **cams, int verbose);
void reportModes(dc1394camera_t *cam);
void reportFeatures(dc1394camera_t *cam);
const char* getColorCodeString(dc1394color_coding_t code);
void reset_bus();

void parseOptions(int argc, char* argv[])
{
    int c;
    while( (c = getopt_long( argc, argv, "", longopts, NULL)) != -1)
    {
        switch(c)
        {
        case 0:
            fwtopts.verbose = 1;
            break;
        case 1:
            fwtopts.camid = strdup(optarg);
            break;
        case 2:
            fwtopts.reset = 1;
            break;
        case 3:
            fwtopts.report = 1;
            if (optarg!=NULL)
                fwtopts.report = atoi(optarg);
            break;
        default:
            printf( "\n"
                    "%s - probe firewire camera for various parameters\n\n"
                    "Usage:\n"
                    "%s [--verbose] [--camid=0xnnnnnnnnnnnn] [--reset] [--help]\n"
                    "    [--report[=level]]\n"
                    "    --verbose: Switch on verbose print out.\n"
                    "    --camid:   Search for and use a camera on the bus\n"
                    "               with the particular ID (12 digit hex value)\n"
                    "    --reset:   Reset the firewire bus using the first camera\n"
                    "               found on each chain (generation) of the bus\n"
                    "    --report   Request a report of the found cameras. Will be\n"
                    "               limited to one camera if --camid has been used.\n"
                    "               The optional \'level\' specifies what type of report\n"
                    "    --help:    This help message\n"
                    "\n", argv[0], argv[0]);
            exit(0);
            break;
        }
    }
    if(fwtopts.verbose==1)
    {
        printf("\nOption parsed:\n"
                "    camid = %s\n"
                "    reset = %d\n"
                "    report = %d\n"
                ,fwtopts.camid, fwtopts.reset, fwtopts.report);
    }
}

int main(int argc,char *argv[])
{

    dc1394_t * bus;
    dc1394camera_list_t * list;
    dc1394camera_t ** cams;
    dc1394camera_t ** tmpcams;
    dc1394camera_t * cam = NULL;
    dc1394camera_t *tmpcam = NULL;
/*    camListItem_t *head, *curr;*/
    dc1394error_t err;
    int i, nReports=0;
    int nAllocatedCams = 0;

    // First parse the options the user requested
    parseOptions(argc, argv);

    // Allocate a handle to the bus
    bus = dc1394_new ();
    if (bus == NULL)
    {
        fprintf(stderr, "ERROR [%s:%d]: could not create dc1394 context.\n", __FILE__, __LINE__);
        return -1;
    }

    // Enumerate all the cameras on the bus (gives us a list of available cameras)
    err=dc1394_camera_enumerate (bus, &list);
    if (err!=0)
    {
        ERR( err );
        dc1394_free (bus);
        return -2;
    }
    printf("Enumerated %d camera(s) on bus\n", list->num);
    cams = (dc1394camera_t **)calloc(list->num + 1, sizeof(dc1394camera_t *));

    // Allocate memory and handle to all the cameras on the bus
    nAllocatedCams = allocate_cams(bus, list, cams, 1);
    if (nAllocatedCams < 1)
    {
        fprintf(stderr, "ERROR [%s:%d] Did not manage to allcoate a single camera\n", __FILE__, __LINE__);
        return -3;
    }

    // Create a linked list with all the cameras we are interested in
    // (currently either all or the specified --camid)
/*  if (fwtopts.camid != NULL)
    {
        cam = findcam(cams, nAllocatedCams, fwtopts.camid);
        if (cam == NULL) return -4;
        head = (camListItem_t*)calloc(1,sizeof(camListItem_t));
        head->camera = cam;
        head->next = NULL;
    } else
    {
        for (i=0; i < nAllocatedCams; i++)
        {
            curr = (camListItem_t*)calloc(1,sizeof(camListItem_t));
            curr->camera =
        }
    }
*/
    if (fwtopts.report > 0)
    {
        if (cam == NULL)
        {
            nReports = nAllocatedCams;
            tmpcams = cams;
        } else
        {
            nReports = 1;
            tmpcams = &cam;
        }

        for (i=0; i<nReports; i++)
        {
            tmpcam = *(tmpcams+i);
            //ERR( dc1394_camera_print_info(tmpcam, stdout) );
            reportModes(tmpcam);
            reportFeatures(tmpcam);
        }

    }

    if (fwtopts.reset) reset_bus();

    dc1394_camera_free_list (list);
    dc1394_free (bus);

    return(0);
}

int allocate_cams(dc1394_t *bus, dc1394camera_list_t *list, dc1394camera_t **cams, int verbose)
{
    unsigned int i, cnt=0;
    dc1394camera_t* camera;
    uint32_t node, generation;
    unsigned int tblWidth = 89;

    for (i=0;i<tblWidth;i++) printf("_");
    printf( "\n");
    printf("%19s |%19s |%25s |%8s |%8s |\n", "GUID", "Vendor", "Model","Node","Gen.");
    for (i=0;i<tblWidth;i++) printf("_");
    printf( "\n");

    // run through all the identified cameras and allocate them
    for (i = 0; i < list->num; i++)
    {
        camera = dc1394_camera_new(bus, list->ids[i].guid);
        if (camera != NULL)
        {
            ERR( dc1394_camera_get_node(camera, &node, &generation) );
            if (verbose > 0)
                printf(" 0x%16.16lX |%19s |%25s |%8d |%8d |\n",
                        camera->guid, camera->vendor, camera->model, node, generation);
            cams[cnt] = camera;
            cnt++;
        }
    }
    for (i=0;i<tblWidth;i++) printf("_");
    printf( "\n");
    return cnt;
}
typedef struct camNode_t {
    ELLNODE node;
    uint32_t generation;
    dc1394camera_t *cam;
}camNode_t;


void reset_bus()
{
    dc1394_t * d;
    dc1394camera_list_t * list;
    dc1394camera_t *cam = NULL;
    uint32_t generation, latch=0;
    uint32_t node;
    ELLLIST camList;
    camNode_t *camListItem, *tmp;
    unsigned int i, newBus;

    d = dc1394_new ();
    ERR( dc1394_camera_enumerate (d, &list) );

    // To reset a multi-bus system it is necessary to find a camera on each
    // individual bus and call the reset function with that camera.
    ellInit(&camList);

    // Get the 'generation' parameter for each camera. This parameter indicate
    // which bus the camera is located on.
    // For each specific 'generation' we add the camera handle to a list which
    // we can later use to reset each bus.
    for (i=0;i<list->num; i++)
    {
        fflush(stdout);
        cam = dc1394_camera_new (d, list->ids[i].guid);
        ERR( dc1394_camera_get_node(cam, &node, &generation) );

        // Run through the collected list of cameras and check if anyone
        // has the same 'generation' parameter... (i.e. is on the same bus)
        tmp=(camNode_t*)ellFirst(&camList);
        newBus = 1;
        while(tmp!=NULL)
        {
            if (generation == tmp->generation)
            {
                newBus = 0;
                break;
            }
            tmp=(camNode_t*)ellNext((ELLNODE*)tmp);
        }

        // If we havent already listed a camera on this bus -or if this is the
        // first camera we check: add the camera handle to a list of cameras that
        // we want to use for resetting busses.
        // Else free up the camera handle as we won't use it until we instantiate
        // our driver plugin.
        if (newBus==1 || i==0)
        {
            camListItem = (camNode_t*)calloc(1, sizeof(camNode_t));
            camListItem->cam = cam;
            camListItem->generation = generation;
            ellAdd(&camList, (ELLNODE*)camListItem);
            latch = generation;
        } else
        {
            // if we dont need the camera handle to reset the bus
            // we might as well free it up
            dc1394_camera_free(cam);
        }
    }

    // Go through the list of cameras that have been identified to be
    // on separate physical busses. Call reset for each of them and free
    // up the camera handle
    camListItem = (camNode_t*)ellFirst(&camList);
    while(camListItem != NULL)
    {
        printf("Resetting bus: %3d using cam: 0x%16.16lX... ", camListItem->generation, camListItem->cam->guid);
        fflush(stdout);
        ERR( dc1394_reset_bus(camListItem->cam) );
        printf("Done\n");
        dc1394_camera_free(camListItem->cam);
        camListItem = (camNode_t*)ellNext((ELLNODE*)camListItem);
    }

    // Clear up after ourselves.
    ellFree(&camList);
    dc1394_camera_free_list (list);
    dc1394_free (d);
    printf("\n");
    return;
}  

dc1394camera_t* findcam(dc1394camera_t **cams, int ncams, char* guid)
{
    int i;
    uint64_t iguid = 0;
    unsigned int ret;

    // First parse the GUI string into a 64bit integer if possible
    if (guid != NULL)
    {
        ret = sscanf(guid, "0x%16lX", &iguid);
        if (ret != 1)
        {
            fprintf(stderr, "ERROR [%s:%d] could not parse GUID string: \"%s\"\n", __FILE__, __LINE__, guid);
            return NULL;
        }
    }

    // run through all the identified cameras on the bus and check if the
    // ID matches the requested one
    for (i = 0; i < ncams; i++)
    {
        if (cams[i]->guid == iguid) return cams[i];
    }

    fprintf(stderr, "Could not find requested camera: %s\n", guid);
    return NULL;
}


void reportModes(dc1394camera_t *cam)
{
    dc1394video_modes_t video_modes;
    dc1394framerates_t framerates;
    dc1394color_coding_t color_coding;
    dc1394color_codings_t codings;
    dc1394bool_t is_scalable;
    dc1394video_mode_t mode;
    uint32_t width, height;
    float framerate;
    unsigned int i,j,q;
    unsigned int tblWidth = 88;

    ERR( dc1394_video_get_supported_modes(cam, &video_modes) );

    printf("\n    Supported modes for camera: 0x%16.16lX %20s\n", cam->guid, cam->model);
    for (i=0;i<tblWidth;i++) printf("_");
    printf( "\n");

    printf("%5s |","mode");
    printf("%12s |","size WxH");
    printf(" scalable |");
    printf(" %15s |", "color coding (id)");
    printf(" %-33s |\n", "framerates (fps)");
    for (i=0;i<tblWidth;i++) printf("_");
    printf( "\n");
    for(i=0; i<video_modes.num; i++)
    {
        mode = video_modes.modes[i];
        ERR( dc1394_get_image_size_from_video_mode(cam, mode, &width, &height) );
        is_scalable = dc1394_is_video_mode_scalable (mode);
        ERR( dc1394_get_color_coding_from_video_mode(cam, mode, &color_coding) );
        if (is_scalable)
        {
            ERR( dc1394_format7_get_max_image_size (cam, mode, &width, &height) );
            ERR( dc1394_format7_get_color_codings (cam,mode, &codings) );
        }

        printf(" %4d |", (unsigned int)mode);
        printf(" %4d x %4d |", width, height);
        is_scalable ? printf("    YES   |") : printf("     NO   |");

        if (!is_scalable)
        {
            printf(" %10s (%4d) |", getColorCodeString(color_coding), (unsigned int)color_coding);
            ERR( dc1394_video_get_supported_framerates(cam, mode, &framerates) );
            for (j=0; j<framerates.num; j++)
            {
                ERR( dc1394_framerate_as_float(framerates.framerates[j], &framerate) );
                printf(" %5.2f ", framerate);
            }
            for (q=0;q<(5-framerates.num);q++) printf("       ");
        } else
        {
            int len = 0;
            for (j=0; j<codings.num; j++)
            {
                len += printf(" %s", getColorCodeString(codings.codings[j]));
            }
            for (q=len;q<55;q++) printf(" ");
        }
        printf("|\n");

    }
    for (i=0;i<tblWidth;i++) printf("_");
    printf( "\n");
}

void reportFeatures(dc1394camera_t *cam)
{
    dc1394featureset_t features;
    ERR( dc1394_feature_get_all (cam, &features) );
    //ERR( dc1394_feature_print_all (&features, stdout) );
}


const char* getColorCodeString(dc1394color_coding_t code)
{
    unsigned int i;

    for (i=0; i<DC1394_COLOR_CODING_NUM; i++)
        if (colorCodeString_lut[i].code == code) return colorCodeString_lut[i].colorCodeStr;
    return "";
}
