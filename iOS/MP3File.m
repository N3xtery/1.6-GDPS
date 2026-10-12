#import "16gdpsIOS.h"
#include <id3tag.h>

@interface MP3File ()
- (void)readTags:(NSString *)path;
@end

// ---------------------------------------------------------------------------

static NSString *StringFromID3Field(union id3_field const *field) {
    if (!field) return nil;
    // TPE1/TIT2 are text-information frames: a non-empty string list whose
    // first entry is the value (multi-value TCOM-style frames aren't used here).
    unsigned count = id3_field_getnstrings(field);
    if (count == 0) return nil;
    id3_ucs4_t const *ucs4 = id3_field_getstrings(field, 0);
    if (!ucs4) return nil;
    id3_utf8_t *utf8 = id3_ucs4_utf8duplicate(ucs4);
    if (!utf8) return nil;
    NSString *value = [NSString stringWithUTF8String:(const char *)utf8];
    free(utf8);
    return [value stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
}

static NSString *ReadTextFrame(struct id3_tag *tag, char const *frameId) {
    struct id3_frame *frame = id3_tag_findframe(tag, frameId, 0);
    if (!frame || frame->nfields < 2) return nil;
    // Field 0 is the text encoding; field 1 is the string list itself.
    return StringFromID3Field(&frame->fields[1]);
}

// ---------------------------------------------------------------------------

@implementation MP3File

@synthesize artist, title;

- (id)initWithPath:(NSString *)path {
    self = [super init];
    if (!self) return nil;

    NSData *data = [NSData dataWithContentsOfFile:path options:NSDataReadingMapped error:NULL];
    if (!data || [data length] < 4) {
        return nil;
    }

    [self readTags:path];

    return self;
}

- (void)readTags:(NSString *)path {
    struct id3_file *file = id3_file_open([path fileSystemRepresentation], ID3_FILE_MODE_READONLY);
    if (!file) return;

    struct id3_tag *tag = id3_file_tag(file);
    if (tag && tag->nframes > 0) {
        NSString *value;
        if ((value = ReadTextFrame(tag, ID3_FRAME_ARTIST)) && [value length] > 0) artist = [value copy];
        if ((value = ReadTextFrame(tag, ID3_FRAME_TITLE))  && [value length] > 0) title  = [value copy];
    }

    id3_file_close(file);
}

@end
