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
	typedef int chan_type;
	std::pair<channel::source<chan_type>, channel::sink<chan_type> >
		p = channel::make_chan<chan_type>();
	p.first.send(1);
	if(p.second.recv() != 1){
		LOG; ++fail;
	}
}

void future_promise()
{
	typedef int chan_type;
	std::pair<channel::promise<chan_type>, channel::future<chan_type> >
		p = channel::make_future<chan_type>();
	p.first = 1;
	if(*p.second != 1){
		LOG; ++fail;
	}
}

void producer(channel::source<int> ch)
{
	std::set<channel::source<int> > source_set;
	source_set.insert(ch);
	channel::select(source_set);
	assert(source_set.size() == 1);
	source_set.begin()->send(0);
	channel::select(source_set);
	assert(source_set.size() == 1);
	source_set.begin()->send(0);
}

void select()
{
	typedef int chan_type;
	std::pair<channel::source<chan_type>, channel::sink<chan_type> >
		p = channel::make_chan<chan_type>(1);
	boost::thread worker(boost::bind(&producer, p.first));
	std::set<channel::sink<chan_type> > sink_set;
	sink_set.insert(p.second);
	channel::select(sink_set);
	assert(sink_set.size() == 1);
	sink_set.begin()->recv();
	channel::select(sink_set);
	assert(sink_set.size() == 1);
	sink_set.begin()->recv();
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
