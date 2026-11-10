/* The movies' player. See movie.h.
 *
 * Both games' movies play through No Regret's Code142 (1468): four frames
 * read ahead, each shown when the sound played reaches it (the first three,
 * and all of a silent movie, by the clock), a late frame decoded and not
 * drawn, the sound queued into AUDSTRM.C's ring. Its code is kept as it was,
 * on the AVI file's chunks (avifile.h) and a decoder of the file's codec
 * (videodecoder.h); the sound chunks before the first frame's (No Remorse's
 * files lead with four) are queued first, as the AVI's interleave has them
 * play from the start. The game holds itself and readies the screen around
 * the movie (MovieHost). */
#include "gamestate.h"
#include "movie.h"
#include "asylum.h"
#include "audconv.h"
#include "audstrm.h"
#include "avifile.h"
#include "clock.h"
#include "host.h"
#include "movie_game.h"
#include "subtitles.h"
#include "systimer.h"
#include "videodecoder.h"
#include "waits.h"
#include "screen.h"

#include <vector>

u8 &movieAborted = gameGlobal<u8>(0);
i16 &doubleMovies = gameSetting<i16>(0);
i16 &movieFrameFlags = gameGlobal<i16>(0);

namespace {

const i16 MOVIE_SLOTS = 4;

/* queueAudio's flags: start the sound, and do not wait for room in the
 * ring. */
const u32 AUDIO_START = 1;
const u32 AUDIO_NO_WAIT = 2;

const u32 VIDEO_FRAME = riffId('0', '0', 'd', 'b');
const u32 VIDEO_DELTA = riffId('0', '0', 'd', 'c');
const u32 SOUND = riffId('0', '1', 'w', 'b');

bool isVideo(u32 id)
{
    return id == VIDEO_FRAME || id == VIDEO_DELTA;
}

/* The clock the frames go by without sound: 60 ticks a second (MOVHOST's). */
const s32 CLOCK_RATE = 60;

s32 clockTime()
{
    return static_cast<s32>(systemTimer->ticks);
}

/* The sound's format as the AVI's 'strf' has it; a Xan sound ('Axan') as
 * AVIREAD.C made it out. */
bool soundFormat(const AviFile &file, WaveFormat &format)
{
    const AviStream *sound = file.stream(riffId('a', 'u', 'd', 's'));
    if ((sound == nullptr) || sound->format.size() < sizeof(WaveFormat)) {
        return false;
    }
    memcpy(&format, sound->format.data(), sizeof(WaveFormat));
    if (sound->handler == riffId('A', 'x', 'a', 'n')) {
        format.formatTag = WAVE_FORMAT_XAN;
        format.blockAlign = format.channels == 1 ? 0x5bf : 0xb7e;
        format.avgBytesPerSec = static_cast<i16>(format.blockAlign * 15);
    }
    return true;
}

/* Turns unsigned 8-bit samples into signed ones. */
void toSigned(u8 *data, i16 count)
{
    for (i16 i = 0; i < count; i++) {
        data[i] += 0x80;
    }
}

class MoviePlayback {
public:
    MoviePlayback(AviFile &newFile, VideoDecoder &newVideo, const MoviePlace &newPlace, const SubtitleTrack *track)
        : file(newFile), video(newVideo), place(newPlace), subtitles(track)
    {
    }
    ~MoviePlayback() { close(); }

    /* The clock, and the sound when there is one and it plays (Code142's
     * open, with a model stream of 16-bit stereo); without a converter for
     * its format the movie plays silent (the original halted). */
    void open(bool sound)
    {
        /* Rounded where the original's division cut it and added a tick: the
         * same 4 ticks for No Regret's 66,666 us a frame, and for No
         * Remorse's 66,667 too (not 5). */
        frameTicks =
            (CLOCK_RATE * static_cast<s32>(file.header().microSecPerFrame) + 500000L) / static_cast<s32>(1000000L);
        if (!sound || !soundFormat(file, wave)) {
            return;
        }
        AudioFormat format;
        memset(&format, 0, sizeof format);
        format.channels = 2;
        format.bitsPerSample = 16;
        format.blockAlign = format.channels << (format.bitsPerSample >> 4);
        format.formatTag = 1;
        format.samplesPerSec = wave.samplesPerSec;
        format.avgBytesPerSec = format.samplesPerSec * format.blockAlign;
        const bool convert = wave.formatTag != 1 || (wave.channels == 1 && wave.bitsPerSample == 8);
        if (!convert) {
            format.channels = wave.channels;
            format.blockAlign = wave.blockAlign;
            format.avgBytesPerSec = format.samplesPerSec * format.blockAlign;
            format.bitsPerSample = wave.bitsPerSample;
        }
        /* Rounded where the original's cut it: 15 for No Regret's 66,666 us a
         * frame and No Remorse's 66,667 alike (not 14, whose 6,300 bytes a
         * frame for its chunks' 5,880 let the frames fall behind the
         * sound). */
        const s32 microSec = static_cast<s32>(file.header().microSecPerFrame);
        const s32 framesPerSec = (static_cast<s32>(1000000L) + microSec / 2) / microSec;
        audioPerFrame = format.avgBytesPerSec / framesPerSec;
        audioBufferSize = 0x8800;
        if (format.samplesPerSec >= 44100) {
            audioBufferSize *= 2;
        }
        /* The ring holds the sound that leads the frames too (No Remorse's
         * four chunks), in DirectPlay's blocks of 80h: without the room the
         * sound runs past what was queued. */
        audioBufferSize += (leadChunks() * audioPerFrame + 0x7f) & ~0x7f;
        AudioRequest request;
        memset(&request, 0, sizeof request);
        request.size = sizeof(request);
        request.format = &format;
        request.bufferSize = audioBufferSize;
        if (audioOpen(&request, &audio) != 1) {
            audio = nullptr;
            return;
        }
        if (convert) {
            converter = openConverter(&wave, &format);
            if (converter == nullptr) {
                close();
                return;
            }
            audioData.resize((format.avgBytesPerSec << 1) / framesPerSec);
        }
    }

    /* The sound closed: its stream waits for the block that plays. */
    void close()
    {
        if (audio != nullptr) {
            audioClose(audio);
            audio = nullptr;
        }
        if (converter != nullptr) {
            closeConverter(converter);
            converter = nullptr;
        }
    }

    [[nodiscard]] bool cancelled() const { return stopped; }

    /* Code142's play, a wait at a time: start before its loop; advance to
     * its next wait for the clock (true: the caller lets it pass and
     * calls again) or to the loop's end, at the movie's end or a key
     * (false); finish after it. */
    void start()
    {
        framesLeft = framesToShow = 0x7fffffffL;
        flush();
        stage = Stage::LEAD;
    }

    bool advance()
    {
        const std::vector<AviFile::Chunk> &all = file.chunks();
        for (;;) {
            switch (stage) {
            case Stage::LEAD:
                /* The sound chunks before the first frame's, queued without
                 * starting the sound (the original's queueLead). */
                if ((audio != nullptr) && cursor < all.size() && all[cursor].id == SOUND) {
                    prepareSound(all[cursor++]);
                    stage = Stage::LEAD_SOUND;
                } else {
                    stage = Stage::TURN;
                }
                break;
            case Stage::LEAD_SOUND:
                switch (tryQueueAudio(pendingData, pendingSize, 0)) {
                case Queued::DONE:
                    stage = Stage::LEAD;
                    break;
                case Queued::FAILED:
                    stage = Stage::TURN;
                    break;
                case Queued::WAIT:
                    return true;
                }
                break;
            case Stage::TURN:
                turnShown = framesShown;
                if ((queueFlags != 0U) && (update() == 0)) {
                    stage = Stage::DONE;
                    return false;
                }
                stage = Stage::TURN_END;
                if (queued() == MOVIE_SLOTS) {
                    /* The original read more of the file into its ring here. */
                } else if (framesLeft <= 0) {
                    if (queued() == 0) {
                        stage = Stage::DONE;
                        return false;
                    }
                } else {
                    frames[writeSlot] = frame;
                    if (const AviFile::Chunk *chunk = readVideo()) {
                        chunks[writeSlot] = chunk;
                        framesLeft--;
                        writeSlot = (writeSlot + 1) & (MOVIE_SLOTS - 1);
                        if (audio != nullptr) {
                            if (const AviFile::Chunk *sound = readAudio()) {
                                prepareSound(*sound);
                                stage = Stage::TURN_SOUND;
                            } else {
                                /* The sound has ended before the frames (No
                                 * Remorse's, which leads them). */
                                soundEnded = true;
                            }
                        }
                    } else {
                        /* The file's frames have all been read. */
                        framesLeft = 0;
                    }
                }
                break;
            case Stage::TURN_SOUND:
                switch (tryQueueAudio(pendingData, pendingSize, queueFlags)) {
                case Queued::DONE:
                    queueFlags = audioPlaying;
                    stage = Stage::TURN_END;
                    break;
                case Queued::FAILED:
                    stage = Stage::DONE;
                    return false;
                case Queued::WAIT:
                    return true;
                }
                break;
            case Stage::TURN_END:
                if ((queueFlags == 0U) && (framesLeft <= 0 || queued() == MOVIE_SLOTS)) {
                    queueFlags = AUDIO_START;
                    if (audio != nullptr) {
                        audioStart(audio);
                        audioPlaying = 1;
                    }
                }
                stage = Stage::TURN;
                /* The queue full, or the last frames left: the next waits for
                 * its time, which the timer and the sound bring. */
                if (turnShown == framesShown && (queued() == MOVIE_SLOTS || framesLeft <= 0)) {
                    return true;
                }
                break;
            case Stage::DONE:
                return false;
            }
        }
    }

    void finish()
    {
        if (audio != nullptr) {
            audioStop(audio);
            audioSeek(audio, 0);
            audioWrite = 0;
        }
    }

private:
    AviFile &file;
    VideoDecoder &video;
    const MoviePlace &place;
    const SubtitleTrack *subtitles;
    /* The file's chunks: the next one, and the frames read. */
    size_t cursor = 0;
    u32 frame = 0;
    WaveFormat wave{};
    s32 framesLeft = 0;
    s32 framesToShow = 0;
    s32 framesShown = 0;
    AudioConverter *converter = nullptr;
    AudioStream *audio = nullptr;
    std::vector<u8> audioData;
    s32 audioBufferSize = 0;
    /* The frames read ahead, from readSlot to writeSlot: the chunk and its
     * frame's number. */
    const AviFile::Chunk *chunks[MOVIE_SLOTS] = {};
    u32 frames[MOVIE_SLOTS] = {};
    i16 readSlot = 0;
    i16 writeSlot = 0;
    i16 audioPlaying = 0;
    u32 audioTotal = 0; /* the bytes queued since flush */
    u32 lastAudioSize = 0;
    u32 audioPending = 0; /* queued and not yet played */
    u32 audioWrite = 0;   /* where the next sound goes in the ring */
    s32 audioPerFrame = 0;
    i16 late = 0; /* the sound runs short: a frame may be skipped */
    s32 frameTicks = 0;
    s32 startTime = 0;
    u16 queueFlags = 0;
    bool stopped = false;
    /* Where advance is in play's loop: the lead's sound, a turn's start, its
     * sound, its end; the turn's frames shown at its start; the sound chunk
     * prepared (prepareSound) and waiting for room in the ring. */
    enum class Stage : u8 { LEAD, LEAD_SOUND, TURN, TURN_SOUND, TURN_END, DONE };
    Stage stage = Stage::LEAD;
    s32 turnShown = 0;
    const u8 *pendingData = nullptr;
    u32 pendingSize = 0;
    /* tryQueueAudio's: queued, room to wait for, or not queued. */
    enum class Queued : u8 { DONE, WAIT, FAILED };
    /* A frame was read without its sound. */
    bool soundEnded = false;

    /* The next frame's chunk (AVIREAD.C's readVideo: the sound before it is
     * passed); nullptr after the last. */
    const AviFile::Chunk *readVideo()
    {
        const std::vector<AviFile::Chunk> &all = file.chunks();
        while (cursor < all.size()) {
            const AviFile::Chunk &chunk = all[cursor++];
            if (isVideo(chunk.id)) {
                frame++;
                return &chunk;
            }
        }
        return nullptr;
    }

    /* The sound chunk before the next frame's, nullptr when there is none
     * (AVIREAD.C's readAudio). */
    const AviFile::Chunk *readAudio()
    {
        const std::vector<AviFile::Chunk> &all = file.chunks();
        while (cursor < all.size()) {
            const AviFile::Chunk &chunk = all[cursor];
            if (isVideo(chunk.id)) {
                return nullptr;
            }
            cursor++;
            if (chunk.id == SOUND) {
                return &chunk;
            }
        }
        return nullptr;
    }

    /* The sound chunks before the first frame's. */
    [[nodiscard]] s32 leadChunks() const
    {
        const std::vector<AviFile::Chunk> &all = file.chunks();
        s32 count = 0;
        while (static_cast<size_t>(count) < all.size() && all[count].id == SOUND) {
            count++;
        }
        return count;
    }

    /* A sound chunk converted for the ring (Code142's play), once: the data
     * and size tryQueueAudio queues (pendingData, pendingSize). */
    void prepareSound(const AviFile::Chunk &sound)
    {
        u32 size = sound.size;
        if (converter != nullptr) {
            size = convertAudio(converter, sound.data, size, audioData.data());
            pendingData = audioData.data();
            pendingSize = size;
            return;
        }
        if (wave.bitsPerSample == 8) {
            toSigned(sound.data, static_cast<i16>(size));
        }
        pendingData = sound.data;
        pendingSize = size;
    }

    /* The slots that hold a frame. */
    [[nodiscard]] i16 queued() const
    {
        if (writeSlot < readSlot) {
            return MOVIE_SLOTS - readSlot + writeSlot;
        }
        if (writeSlot == readSlot) {
            return (chunks[writeSlot] != nullptr) ? MOVIE_SLOTS : 0;
        }
        return writeSlot - readSlot;
    }

    void flush()
    {
        memset(frames, 0, sizeof(frames));
        while (chunks[readSlot] != nullptr) {
            chunks[readSlot] = nullptr;
            readSlot = (readSlot + 1) & (MOVIE_SLOTS - 1);
        }
        readSlot = writeSlot = 0;
        audioPlaying = 0;
        audioTotal = lastAudioSize = audioPending = 0;
        late = 0;
        startTime = 0;
        framesShown = 0;
        queueFlags = 0;
    }

    i16 update()
    {
        i16 result = 1;

        if ((audioPlaying != 0) && framesShown > 3) {
            u32 position;

            clockPushHold();
            clockHold();
            const u16 left = ASS_GetDmaBytesLeft();
            audioPosition(audio, &position);
            clockPopHold();
            if (audioWrite >= position) {
                audioPending = audioWrite - position;
            } else {
                audioPending = audioBufferSize + audioWrite - position;
            }
            audioPending += left;
            startTime = 0;
            /* Short of sound, a frame is skipped; not once the sound has
             * ended (No Remorse's last frames). */
            if (static_cast<u32>(3 * audioPerFrame) > audioPending && !soundEnded) {
                late = 1;
            }
            const u32 due = framesShown * audioPerFrame;
            if (audioTotal - audioPending < due) {
                return result;
            }
        } else {
            const s32 now = clockTime();

            if (startTime == 0) {
                startTime = now;
            }
            const s32 elapsed = now - startTime;
            if (frameTicks <= elapsed) {
                if (chunks[readSlot] == nullptr) {
                    return result;
                }
                startTime = now;
            } else {
                return result;
            }
        }
        if (chunks[readSlot] != nullptr) {
            result = showFrame(framesLeft > 0 && (late != 0));
            /* The movies' baseline of frames. */
            hostMovieFrame(static_cast<u32>(framesShown));
            chunks[readSlot] = nullptr;
            readSlot = (readSlot + 1) & (MOVIE_SLOTS - 1);
            framesToShow--;
            framesShown++;
            late = 0;
        }
        if ((framesLeft <= 0 || soundEnded) && (audio != nullptr)) {
            silence();
        }
        return result;
    }

    /* XANPLAY.C's showFrame: the frame decoded and, unless late, drawn; the
     * subtitle that appears at it; 0 when a key cancels the movie. */
    i16 showFrame(bool lateFrame)
    {
        video.decode(*chunks[readSlot], lateFrame);
        if (!lateFrame) {
            video.show(place);
        }
        if (subtitles != nullptr) {
            if (const SubtitleCue *cue = subtitles->appearing(frames[readSlot])) {
                drawSubtitle(SUBTITLE_STYLE, cue->text, place.bottom(), video.lightColour(), video.darkColour(),
                             place.hiColour);
            }
        }
        if (movieInput().skipRequested()) {
            stopped = true;
            return 0;
        }
        return 1;
    }

    /* A try of queueAudio's loop: the data queued, or room to wait for
     * while the sound plays on to make it (the caller lets the clock pass
     * and tries again), or not queued (no ring, or no room with
     * AUDIO_NO_WAIT). */
    Queued tryQueueAudio(const u8 *data, u32 size, u32 flags)
    {
        u8 *ring;
        u32 position;
        u32 first;
        u32 second;
        u32 space;

        audioPending = 0;
        if (audioBuffer(audio, &ring) != 1) {
            return Queued::FAILED;
        }
        audioPosition(audio, &position);
        if (audioWrite >= position) {
            space = audioBufferSize - audioWrite + position;
        } else {
            space = position - audioWrite;
        }
        if (space <= size) {
            audioUpdate(audio);
            if ((audioPlaying == 0) && ((flags & AUDIO_NO_WAIT) == 0U)) {
                audioStart(audio);
                audioPlaying = 1;
            }
            if ((flags & AUDIO_NO_WAIT) != 0U) {
                return Queued::FAILED;
            }
            return Queued::WAIT;
        }
        first = audioBufferSize - audioWrite < size ? audioBufferSize - audioWrite : size;
        second = size - first;
        memcpy(ring + audioWrite, data, first);
        if (second != 0U) {
            memcpy(ring, data + first, second);
            audioWrite = second;
        } else {
            audioWrite += first;
        }
        audioUpdate(audio);
        if ((audioPlaying == 0) && ((flags & AUDIO_START) != 0U)) {
            audioStart(audio);
            audioPlaying = 1;
        }
        lastAudioSize = size;
        audioTotal += size;
        audioPending = audioBufferSize - space + size;
        return Queued::DONE;
    }

    /* Queues an eighth of the ring of silence, if there is room. */
    void silence()
    {
        std::vector<u8> data(static_cast<size_t>(audioBufferSize) / 8, 0);
        static_cast<void>(tryQueueAudio(data.data(), static_cast<u32>(data.size()), AUDIO_START | AUDIO_NO_WAIT));
    }
};

/* Where the movie goes: centred or at the request's place, each pixel 2
 * across when the configuration asks and it fits, on the 16-bit screen when
 * it asks and the decoder draws there. */
MoviePlace placeOf(const MovieRequest &request, const AviHeader &header, const MovieSettings &settings,
                   const VideoDecoder &video)
{
    MoviePlace place;
    place.width = static_cast<u16>(header.width);
    place.height = static_cast<u16>(header.height);
    place.doubled =
        request.mayDouble && settings.big && header.width * 2 <= SCREEN_WIDTH && header.height * 2 <= SCREEN_HEIGHT;
    place.hiColour = settings.hiColour && video.drawsHiColour();
    place.background = request.background;
    const s32 scale = place.doubled ? 2 : 1;
    if (request.centred) {
        place.x = static_cast<i16>((SCREEN_WIDTH - static_cast<s32>(header.width) * scale) / 2);
        place.y = static_cast<i16>((SCREEN_HEIGHT - static_cast<s32>(header.height) * scale) / 2);
    } else {
        place.x = request.x;
        place.y = request.y;
    }
    return place;
}

/* A movie's run across its waits: its request (its name the run's own:
 * the caller's may be gone), file, decoder and playback, from its start
 * to its end; the playback goes first. */
struct MovieRun {
    std::string name;
    MovieRequest request;
    MovieSettings settings;
    AviFile file;
    std::unique_ptr<VideoDecoder> video;
    SubtitleTrack track;
    bool subtitles = false;
    MoviePlace place;
    std::unique_ptr<MoviePlayback> playback;
};

/* The playback's loop as a wait: a pass to its next wait for the clock. */
class PlaybackWait final : public GameWait {
public:
    explicit PlaybackWait(MoviePlayback &playback) : playback_(playback) {}
    void pass() override
    {
        if (playback_.advance()) {
            clockWait();
        } else {
            setOver();
        }
    }
    [[nodiscard]] bool run() override
    {
        playback_.finish();
        return false;
    }

private:
    MoviePlayback &playback_;
};

using MovieRest = std::unique_ptr<GameRest>;
using MovieStage = void (*)(std::unique_ptr<MovieRun> run, MovieRest rest);

/* The movie's end: rest (the caller's) on whether a key stopped it. */
void movieEnd(bool cancelled, MovieRest rest)
{
    setRestResult(cancelled ? 1 : 0);
    static_cast<void>(rest->run());
}

/* After a stage's call that may have opened a wait (the host's, the
 * sound's): next at once, or after the wait. */
void movieOn(std::unique_ptr<MovieRun> run, MovieRest rest, MovieStage next)
{
    if (waitOpened()) {
        giveRest([run = std::move(run), rest = std::move(rest), next]() mutable {
            next(std::move(run), std::move(rest));
            return false;
        });
        return;
    }
    next(std::move(run), std::move(rest));
}

void movieClosed(std::unique_ptr<MovieRun> run, MovieRest rest)
{
    const bool cancelled = run->playback->cancelled();
    movieHost().end(run->request, run->place, cancelled);
    run.reset();
    movieEnd(cancelled, std::move(rest));
}

void moviePlayed(std::unique_ptr<MovieRun> run, MovieRest rest)
{
    movieInput().end();
    run->playback->close();
    movieOn(std::move(run), std::move(rest), movieClosed);
}

void movieReady(std::unique_ptr<MovieRun> run, MovieRest rest)
{
    movieInput().begin();
    run->playback->start();
    MoviePlayback &playback = *run->playback;
    auto played = [run = std::move(run), rest = std::move(rest)]() mutable {
        moviePlayed(std::move(run), std::move(rest));
        return false;
    };
    openWait(std::make_unique<PlaybackWait>(playback), std::make_unique<CodeRest<decltype(played)>>(std::move(played)));
}

void movieBegun(std::unique_ptr<MovieRun> run, MovieRest rest)
{
    run->video->start(run->place);
    run->playback =
        std::make_unique<MoviePlayback>(run->file, *run->video, run->place, run->subtitles ? &run->track : nullptr);
    run->playback->open(run->settings.sound);
    movieHost().ready(run->request);
    movieOn(std::move(run), std::move(rest), movieReady);
}

}

void openMovieFrame(const MovieRequest &request, std::unique_ptr<GameRest> rest)
{
    MovieHost &host = movieHost();
    auto run = std::make_unique<MovieRun>();
    run->name = request.name;
    run->request = request;
    run->request.name = run->name.c_str();
    MovieFiles files;
    if (!host.locate(run->request.name, files)) {
        movieEnd(false, std::move(rest));
        return;
    }
    run->settings = host.settings();
    if (!run->file.open(files.movie.c_str())) {
        movieEnd(false, std::move(rest));
        return;
    }
    run->video = makeVideoDecoder(run->file, run->settings);
    if (!run->video) {
        movieEnd(false, std::move(rest));
        return;
    }
    run->subtitles = run->request.maySubtitle && run->settings.subtitles &&
                     (files.textSubtitles ? readTextSubtitles(files.subtitles.c_str(), run->track)
                                          : readIffSubtitles(files.subtitles.c_str(), "ENGL", run->track));
    if (run->subtitles) {
        /* The lines in the game's language. */
        translateSubtitles(run->track);
    }
    run->place = placeOf(run->request, run->file.header(), run->settings, *run->video);
    host.begin(run->request, run->place);
    movieOn(std::move(run), std::move(rest), movieBegun);
}

bool playMovie(const MovieRequest &request)
{
    bool cancelled = false;
    blockWaits();
    openMovie(request, [&cancelled](bool stopped) { cancelled = stopped; });
    unblockWaits();
    return cancelled;
}
