/*
 * The simulator's media mapping.
 *
 * hwsim/media.h
 *
 * A drive in the machine holds a medium, and the media changer is how
 * the world outside the machine says which.  This is the half of that a
 * card uses: a card with removable drives offers them here, once, from
 * its startup hook, and then answers when the changer tells it to load
 * or eject one.  mediachg.c is the other half - the mapping itself and
 * the port that commands it.
 *
 * The direction matters and it is the reason this is not simply a table
 * of paths.  The card owns the image, because the card is what reads it:
 * a djdma drive's image lives in the card's own handle, opened with the
 * card's own notion of the medium.  So the changer never opens a file
 * behind the card's back.  It says "unit 2 now holds this", the card
 * does the opening, and the changer remembers only where in the set the
 * unit is.
 *
 * A card that never registers is a card with no removable media, and no
 * command can name its drives.
 */
#ifndef MEDIA_H
#define MEDIA_H

struct mediadev {
    char *card;                 /* the name a command uses: "djdma" */
    int   instance;             /* which of that card; 0 unless there are two */
    int   nunits;               /* how many drives it offers */
    int (*load)(int unit, char *path);  /* hold this image there; -1 refused */
    void (*eject)(int unit);            /* let go of it: the drive goes not ready */
};

/*
 * Offer a card's drives.  Called from the card's startup hook, so the
 * unit numbers are the ones the guest sees and the mapping from those to
 * whatever the board calls them stays the card's business.
 */
void register_mediadev(struct mediadev *m);

/*
 * And the other way: a card saying what a drive turned out to hold without
 * the changer having said so.  A drive argument on the command line, a
 * `drives` key in the machine file, a -D host override - all of them put
 * an image in a drive before any command runs, and the changer would
 * otherwise report that drive empty while a disk sat in it.  Called where
 * the card opens such an image; the path is copied.
 */
void media_bound(struct mediadev *m, int unit, char *path);

#endif
