//include
#include <channel.hpp>
#include <logger.hpp>
#include <unit_test.hpp>

int fail(0);

void primitive()
{
	channel::primitive<int> ch;
	ch.send(1);
	if(ch.recv() != 1){
		LOG; ++fail;
	}
	//composition
	channel::primitive<channel::primitive<int> > composed_ch;
	ch.send(1);
	composed_ch.send(ch);
	ch = composed_ch.recv();
	if(ch.recv() != 1){
		LOG; ++fail;
	}
}

void source_sink()
{
	channel::source<int> Source;
	channel::sink<int> Sink(Source.get_sink());
	Source.send(1);
	if(Sink.recv() != 1){
		LOG; ++fail;
	}
}

void future_promise()
{
	channel::promise<int> Promise;
	channel::future<int> Future(Promise.get_future());
	Promise = 1;
	if(*Future != 1){
		LOG; ++fail;
	}
}

void producer(channel::source<int> Source)
{
	std::set<channel::source<int> > complete_set, ready_set;
	complete_set.insert(Source);
	channel::select(complete_set, ready_set);
	assert(ready_set.size() == 1);
	complete_set.begin()->send(0);
	channel::select(complete_set, ready_set);
	assert(ready_set.size() == 1);
	complete_set.begin()->send(0);
}

void select()
{
	channel::source<int> Source(1);
	channel::sink<int> Sink(Source.get_sink());
	boost::thread worker(boost::bind(&producer, Source));
	std::set<channel::sink<int> > complete_set, ready_set;
	complete_set.insert(Sink);
	channel::select(complete_set, ready_set);
	assert(ready_set.size() == 1);
	ready_set.begin()->recv();
	channel::select(complete_set, ready_set);
	assert(ready_set.size() == 1);
	ready_set.begin()->recv();
	worker.join();
}

int main()
{
	unit_test::timeout();
	primitive();
	source_sink();
	future_promise();
	select();
	return fail;
}
