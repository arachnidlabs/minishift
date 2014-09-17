import heapq
import logging
import minishift
import time
import threading
import tweepy


col_delay = 0.015
track_keywords = ['arachnidlabs', 'ukmakerfaire', 'makerfaireuk']
follow_ids = ['1112628326', '21410789']
labs_messages = ["Tweet @arachnidlabs to see your message here!", "Arachnid Labs, makers of fine electronics since 2013", "I'm for sale! Ask my Maker about me!"]
blacklist = set(["piworkshop", "thepixomat"])

logging.basicConfig(level=logging.DEBUG)
ms = minishift.Minishift(minishift.MCP2210Interface(), 9 * 8)


class StreamListener(tweepy.StreamListener):
    def __init__(self, tweets, limit):
        super(StreamListener, self).__init__()
        self.tweets = tweets
        self.limit = limit

    def on_status(self, status):
        if status.author.screen_name in blacklist:
            return
        tweet = "%s - @%s" % (status.text, status.author.screen_name)
        self.tweets.append(tweet)
        logging.debug("Got tweet: %r", tweet)
        if len(self.tweets) > self.limit:
            self.tweets.pop(0)


def twitter_thread(tweets, limit):
    stream = tweepy.Stream(auth, StreamListener(tweets, limit), timeout=None)
    stream.filter(track=track_keywords, follow=follow_ids)


auth = tweepy.auth.OAuthHandler('dhtu64hKE53ff4mtozwJpJA4d', 'yJqLA1IkUOvw7d2b2SGzpS2pqCdLMWUgOsTWfj4OvBKtJsIDJx')
auth.set_access_token('1112628326-eo78B37OkUaYGHytp8VxGtsrKcGhZok8OobGbTR', 'sLbbTgdKVZ4lp81Im1gCNeiJD0SfYo0xEzIPHuzImKvdr')


def tweet_iterator(tweets):
    i = 0
    while True:
        yield tweets[i]
        i += 1
        if i >= len(tweets):
            i = 0


def message_iterator(messages):
    while True:
        for message in messages:
            yield message


def main():
    # Run a separate thread to consume the Twitter stream
    tweets = [""]
    thread = threading.Thread(target=twitter_thread, args=(tweets, 5))
    thread.daemon = True
    thread.start()

    sources = {
        'tweet': tweet_iterator(tweets),
        'message': message_iterator(labs_messages),
    }

    heap = []
    for source in sources:
        heapq.heappush(heap, (0, source, sources[source].next()))

    while True:
        heap_ctr, source, message = heapq.heappop(heap)
        heapq.heappush(heap, (heap_ctr + 1, source, sources[source].next()))
        logging.debug("Writing message: %s", message)

        canvas = minishift.Canvas()
        canvas.write_text(0, message.encode('ascii', errors='ignore') + "  ")
        for col in canvas.scroll():
            ms.update(col)
            time.sleep(col_delay)


if __name__ == '__main__':
    main()
