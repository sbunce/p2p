template<class T>
class ring
{
private:
	//holds the information needed to keep track of the ring
	class ringNode
	{
	public:
		T * ringElement;

		ringNode * next;
		ringNode * prev;
	};

	ringNode * ringHolder; //current position of the ring

public:
	//spins the ring forward or backward by one
	T & operator ++(int count); //post-spin (returns then spins)
	T & operator --(int count); //post-spin (returns then spins)

	ring();
	~ring();
	/*
	current - returns the reference what ringHolder contains.
	empty   - returns true if the ring is empty
	pop     - remove what ringHolder points to
	push    - add a element in front of ringHolder then set ringHolder equal to it
	*/
	T & current();
	bool empty();
	void pop();
	void push(T & T_in);
};

template<class T>
ring<T>::ring()
{
	ringHolder = 0;
}

template<class T>
ring<T>::~ring()
{
	while(!empty()){
		pop();
	}
}

template<class T>
T & ring<T>::operator++(int count)
{
	ringHolder = ringHolder->next;
	return *ringHolder->ringElement;
}

template<class T>
T & ring<T>::operator--(int count)
{
	ringHolder = ringHolder->prev;
	return *ringHolder->ringElement;
}

template<class T>
T & ring<T>::current()
{
	return *ringHolder->ringElement;
}

template<class T>
bool ring<T>::empty()
{
	return ringHolder == 0;
}

template<class T>
void ring<T>::pop()
{
	//if the only node in the tree
	if(ringHolder->next == ringHolder){
		delete ringHolder->ringElement;
		delete ringHolder;
		ringHolder = 0;
	}
	else if(ringHolder == 0){ //if ring empty
		//do nothing!
	}
	else{ //ring is bigger than one and not empty
		ringNode * nextTemp = ringHolder->next;
		ringNode * prevTemp = ringHolder->prev;

		//remove by changing pointers
		nextTemp->prev = prevTemp;
		prevTemp->next = nextTemp;

		//delete orphaned node
		delete ringHolder->ringElement;
		delete ringHolder;

		//reposition ringHolder to a node that exists
		ringHolder = nextTemp;
	}
}

template<class T>
void ring<T>::push(T & T_in)
{
	if(ringHolder != 0){ //add another node to the ring
		//create new node
		ringNode * newNode = new ringNode();

		//store the data in the node
		T * newT = new T();
		*newT = T_in;
		newNode->ringElement = newT;

		//back up pointer to next ringContainer and ringHolder
		ringNode * nextTemp = ringHolder->next;

		//insert by changing pointers
		ringHolder->next = newNode;
		nextTemp->prev = newNode;
		newNode->prev = ringHolder;
		newNode->next = nextTemp;
	}
	else{ //add the first node in the ring

		//create new node
		ringNode * newNode = new ringNode();

		//store the data in the node
		T * newT = new T();
		*newT = T_in;
		newNode->ringElement = newT;

		//since this is the only node next and prev point at itself
		newNode->next = newNode;
		newNode->prev = newNode;

		//set the ringHolder to the only node
		ringHolder = newNode;
	}
}

