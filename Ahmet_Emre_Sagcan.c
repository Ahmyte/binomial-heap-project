#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#define MAX_CONST 1000 //It is my constant, i use it to subtract from the relevancy to build a min heap but if i can subtract from that too i get the same relevancy using a min heap.

typedef struct Document
{ //Degree for how much child does that root have, relevancy is the key of the node, fileName just holds the name of the file
    int degree, relevancy;
    char fileName [200];
    struct Document *parent, *child, *sibling; //Parent points to the parent of that node, child points to the child of that node, sibling points to the sibling of that node,
}Document;

typedef Document* DocumentPtr;

//Function declarations
DocumentPtr CreateDocument(DocumentPtr docPtr, int relevancy, char fileName[200]);
void CheckRelevancy(struct dirent* folder, int *relevancy, int stringSize, FILE *fptr, char keyword[stringSize],char word[stringSize], char checkers[5][2]);
DocumentPtr InsertToBinomHeap(DocumentPtr head, DocumentPtr element);
DocumentPtr CombineTwoHeaders(DocumentPtr firstHeader, DocumentPtr secondHeader);
DocumentPtr MergeBinomHeaps(DocumentPtr firstHeader, DocumentPtr secondHeader);
void ParentificateAndLink(DocumentPtr childDoc, DocumentPtr parentDoc);
DocumentPtr ExtractMinElement(DocumentPtr head);
void ConstructChildrenHeap(DocumentPtr childrenOfMin);
void InsertElementsOf(DocumentPtr head);

DocumentPtr Header = NULL; //This is the header of the heap we should access from everywhere.

int main()
{
    int relevancy, numOfRelevantDocuments = 0;
    int size = 0, stringSize = 50;
    char word[stringSize], keyword[stringSize];
    char checkers [5][2] = {",", ":", ".", "!", "-"}; //These are the punctuations that i am ignoring, the others are somehow blocking my code i can't fix it.
    FILE *fptr = NULL;
    DIR *dir;
    struct dirent *folder;
    dir = opendir("files"); //Opening the files directory in the place where exe file is.

    printf("Enter the keyword:"); //Taking the keyword from user.
    scanf("%s", keyword);
    if (dir != NULL)
    {//Counts the number of files to make a consistent array.
        for(int i=0; (folder = readdir(dir)) != NULL; i++)
        {
            if(i < 2) continue;
            size++;
        }
    }

    DocumentPtr docx[size];
    //Firstly making every element NULL to avoid any type of problem
    for(int i = 0; i < size; i++)
        docx[i] = NULL;

    closedir(dir);

    dir = opendir("files");
    //Reading the folder containing the documents
    if (dir != NULL)
    {
        for(int i=0; (folder = readdir(dir)) != NULL; i++)
        {//For every folder checks the relevancy of that file in CheckRelevancy function
            if(i < 2) continue;
            DocumentPtr docPtr = NULL;
            CheckRelevancy(folder, &relevancy, stringSize, fptr, keyword, word, checkers);
            docx[i-2] = CreateDocument(docPtr, relevancy, folder->d_name); //Fills the array with each document so that i can reach them quickly.
        }
    }
    else
    {
        printf("Cannot open directory.");
        return -1;
    }
    closedir(dir);
    for(int i=0; i < size; i++) //Insert every element of array to the heap.
    {
        Header = InsertToBinomHeap(Header, docx[i]);
        if(MAX_CONST - docx[i]->relevancy > 0)
            numOfRelevantDocuments++;//The relevant ones are counted here.
    }
    printf("\nThere are %d number of relevant documents.\n\nMost relevant ones are:\n" , numOfRelevantDocuments);
    DocumentPtr currentMin = NULL; //This will hold the current minimum document.
    DocumentPtr minDocx[5];//This holds every minimum root in the file.
    for(int i=0; i < 5; i++)
    {
        currentMin = ExtractMinElement(Header); //Extracts the minimum element and assigns it as the current minimum document.
        if(currentMin != NULL && MAX_CONST - currentMin->relevancy > 0)
        {//If it has relevancy more than zero and there is actually anything returned from the extract function, this part executes.
            printf("%s (%d) \n", currentMin->fileName, MAX_CONST - currentMin->relevancy);
            minDocx[i] = currentMin; //Fill the array with minimum files.
        }
    }
    //This part is to print the most relevant files .
    FILE *fptr2 = NULL;
    char fileDir[stringSize];
    char wholeLine[30000];
    int lineCount = 0;
    puts("These documents are as follows (in folder order):");
    dir = opendir("files");
    if (dir != NULL)
    {
        while((folder = readdir(dir)) != NULL)
        {//For every file in the folder, program gets the line with strings in the file and prints it as output.
            sprintf(fileDir, "%s%s", "files/", folder->d_name);
            fptr2 = fopen(fileDir,"r");
            lineCount = 0;
            while(fgets(wholeLine,30000,fptr2))
            {
                for(int i = 0; i < 5; i++)
                {
                    //We check whether they are the same by comparing their file names.
                    if(strcmp(minDocx[i]->fileName,folder->d_name)==0 && lineCount == 0)
                    {
                        printf("\n%s (%d): %s",minDocx[i]->fileName,MAX_CONST- minDocx[i]->relevancy,wholeLine);
                        lineCount++;//Trying to count the lines to make output in the console look better.
                    }
                    else if(strcmp(minDocx[i]->fileName,folder->d_name)==0 && lineCount > 0)
                    {
                        printf("%s",wholeLine);
                        lineCount++;
                    }
                }
            }
        }
        closedir(dir);
    }
    free(fptr);
    free(fptr2);
    return 0;
}

DocumentPtr CreateDocument(DocumentPtr docPtr, int relevancy, char fileName[200])
{//Assing every fields of the struct document with the given variables.
    docPtr = malloc(sizeof(Document));
    docPtr->degree = 0;
    //Subtracting from a bigger constant to make a min heap that holds maximums in roots. If we subtract from it again we'll get the actual number.
    docPtr->relevancy = MAX_CONST - relevancy;
    strcpy(docPtr->fileName, fileName);
    docPtr->parent = docPtr->child = docPtr->sibling = NULL;
    return docPtr;
}

void CheckRelevancy(struct dirent* folder, int *relevancy, int stringSize, FILE *fptr, char keyword[stringSize],char word[stringSize], char checkers[5][2])
{
    //This part checks the relevancies of each file in each call to this function.
    //tempWords hold tokenized strings in the word variable.
    char tempWord[stringSize], tempWord2[stringSize];
    char valueKeeper[stringSize]; //This keeps the value of the keyword in case of any loss of keyword value.(Which i strangely encountered but then i solved using this.)
    strcpy(valueKeeper, keyword);

    //Opens the file in the directory.
    char fileDirectory[stringSize];
    sprintf(fileDirectory, "%s%s", "files/", folder->d_name);
    fptr = fopen(fileDirectory,"r");
    *relevancy = 0;
    int controller=0; //This prevents the program to accidentally read the found keyword 2 times more.
    while(fscanf(fptr, "%s",word) == 1)
    {//Takes every string ending with a space.
        controller = 0;
        for(int j=0; word[j] && controller != 2; j++)
        {//Checks and compares every character with the checkers string array (which holds the punctuations we need to check.
            for (int k = 0; checkers[k][0] && controller != 2; k++)
            {
                if(word[0] != '.' && word[j] == checkers[k][0])
                {
                    strcpy(tempWord,word);//Keeping it in the tempword in case of any change in the word won't affect the result.
                    strtok(tempWord, checkers[k]);
                    if(word[j+1] != '\0' && word[j+1] != '.' && word[j+1] != '!' && word[j+1] != '-')
                        strcpy(tempWord2,strtok(NULL, checkers[k]));

                    strcpy(keyword,valueKeeper);
                    //Checks the two tempWord whether they contain the keyword in them after tokenizing by punctuations.
                    if(stricmp(tempWord,keyword) == 0)
                    {
                        controller++;
                        (*relevancy)++;
                    }
                    if(stricmp(tempWord2,keyword) == 0)
                    {
                        controller++;
                        (*relevancy)++;
                    }
                    //Nullifies because we will probably use them again.
                    strcpy(tempWord,"\0");           strcpy(tempWord2,"\0");
                }
            }
        }
        //Finally checks the relevancy of the taken strin. If there isn't any punctions in the string and if the keyword matches the word this increments relevancy of that file.
        if(stricmp(word,keyword) == 0)          (*relevancy)++;
    }
    fclose(fptr);
}

DocumentPtr InsertToBinomHeap(DocumentPtr head, DocumentPtr element)
{//This function simply calls CombineTwoHeaders which actually inserts the document to the heap.
    DocumentPtr nodeToHeader=NULL;
    nodeToHeader = element;
    head = CombineTwoHeaders(head, nodeToHeader);
    return head;
}

DocumentPtr CombineTwoHeaders(DocumentPtr firstHeader, DocumentPtr secondHeader)
{
    //These are some temp variables to hold the nodes.
    DocumentPtr prevElement = NULL, nextElement = NULL;
    DocumentPtr headerKeeper=NULL;
    headerKeeper = MergeBinomHeaps(firstHeader, secondHeader); //Merges the two headers in that function.
    if(headerKeeper == NULL)
        return headerKeeper;
    DocumentPtr currentElement = headerKeeper;
    nextElement = currentElement->sibling;
    //Now the element is inserted to the heap but there can be two binomial tree with the same degree. This part corrects that.
    while(nextElement != NULL)
    {
        //If these conditions hold then this means it is okay so no need to merge two binomial trees into one.
        if((currentElement->degree != nextElement->degree) || ((nextElement->sibling != NULL) && (nextElement->sibling)->degree == currentElement->degree))
        {
            prevElement = currentElement;
            currentElement = nextElement;
        }
        //But if not then this part is executed.
        else
        {
            //According to their relevancies decides which element should be the root of the two binomial tree.
            if(currentElement->relevancy <= nextElement->relevancy)
            {
                currentElement->sibling = nextElement->sibling;
                ParentificateAndLink(nextElement, currentElement);
            }
            else
            {
                if(prevElement == NULL)
                    headerKeeper=nextElement;
                else
                    prevElement->sibling=nextElement;
                ParentificateAndLink(currentElement, nextElement);
                currentElement=nextElement;
            }
        }
        nextElement=currentElement->sibling; //Traverses each time to find the binomial trees with same degree.
    }
    return headerKeeper;
}

//This function merges the header with the inserted node.
DocumentPtr MergeBinomHeaps(DocumentPtr firstHeader, DocumentPtr secondHeader)
{
    //These are some temp variables to hold the nodes.
    DocumentPtr mergeHeader=NULL;
    DocumentPtr tempSibling1 = NULL, tempSibling2 = NULL;

    DocumentPtr rootTraveller = firstHeader;
    DocumentPtr elementHolder = secondHeader;//Second header is the node to be inserted. Variable holds that.

    //Program decides which root should be chosen by comparing their degrees.
    if(rootTraveller != NULL)
    {
        if(elementHolder != NULL && rootTraveller->degree <= elementHolder->degree)
            mergeHeader = rootTraveller;
        else if(elementHolder != NULL && rootTraveller->degree > elementHolder->degree)
            mergeHeader = elementHolder;
        else
            mergeHeader = rootTraveller;
    }
    else
        mergeHeader = elementHolder;

    //This part arranges the siblings' order. And inserts the elements according to that.
    while(rootTraveller != NULL && elementHolder != NULL)
    {
        if(rootTraveller->degree < elementHolder->degree)
        {
            rootTraveller = rootTraveller->sibling;
        }
        else if(rootTraveller->degree == elementHolder->degree)
        {
            tempSibling1 = rootTraveller->sibling;
            rootTraveller->sibling = elementHolder;
            rootTraveller = tempSibling1;
        }
        else
        {
            tempSibling2 = elementHolder->sibling;
            elementHolder->sibling = rootTraveller;
            elementHolder = tempSibling2;
        }
    }
    return mergeHeader;
}

void ParentificateAndLink(DocumentPtr childDoc, DocumentPtr parentDoc)
{//This just builds a relation between child and parent in code. Also degree is incremented because parent has got another child.
    childDoc->parent = parentDoc;
    childDoc->sibling = parentDoc->child;
    parentDoc->child = childDoc;
    parentDoc->degree++;
}

DocumentPtr HeadOfChildrenList = NULL;

DocumentPtr ExtractMinElement(DocumentPtr head)
{//These are some temp variables to hold the nodes.
    DocumentPtr previousOfMin = NULL;
    DocumentPtr siblingTraveller = NULL;
    HeadOfChildrenList = NULL;

    DocumentPtr minRoot = head; //Min root will travel the siblings so it should be assigned to header.
    if(minRoot == NULL)
    {
        printf("\nThere isn't any element left in the binomial heap.");
        return minRoot;
    }
    siblingTraveller = minRoot;
    int min = minRoot->relevancy;

    //Compares the relevancies of siblings until it finds the minium one.
    while(siblingTraveller->sibling != NULL)
    {
        if((siblingTraveller->sibling)->relevancy < min)
        {
            min = (siblingTraveller->sibling)->relevancy;
            previousOfMin = siblingTraveller;
            minRoot = siblingTraveller->sibling;
        }
        siblingTraveller = siblingTraveller->sibling;
    }
    //minRoot should be equal to the root with minimum key by this point.
    if(minRoot->child != NULL)
    {
        ConstructChildrenHeap(minRoot->child); //Constructs the children heap
        (minRoot->child)->sibling = NULL; //We should do this, in order to prevent loops inside this children heap.
    }

    if(previousOfMin == NULL && minRoot->sibling == NULL)
    {//If the minimum is not sibling of an element and if the minimum has no element execute this.
        head = NULL;
        Header = HeadOfChildrenList;
    }
    else if(previousOfMin == NULL)
    {//If the minimum root is the most left child execute this.
        head = minRoot->sibling;
        Header = head;//Change of the header is necessary beacsue we are inserting children to header one by one.
        InsertElementsOf(HeadOfChildrenList);
    }
    else if(previousOfMin->sibling == NULL)
    {//If the previous one does not have a sibling execute this.
        previousOfMin = NULL;
        InsertElementsOf(HeadOfChildrenList);
    }
    else
    {//If it is any other case other than above statements execute this.
        previousOfMin->sibling = minRoot->sibling;
        InsertElementsOf(HeadOfChildrenList);
    }
    return minRoot;
}

void ConstructChildrenHeap(DocumentPtr childrenOfMin)
{//Constructs an heap from the extracted root's child and child's sibling. Also reverses the sibling links.
    if(childrenOfMin->sibling != NULL)
    {
        ConstructChildrenHeap(childrenOfMin->sibling);
        (childrenOfMin->sibling)->sibling=childrenOfMin;
    }
    else
        HeadOfChildrenList = childrenOfMin; //Finds the head of children list and assigns it.
}

void InsertElementsOf(DocumentPtr head)
{//This function inserts elements of the given head but not the head because it is going to be extracted from the heap.
    DocumentPtr currentElement = NULL; //This holds the current element in the each heap traversal.
    while (head != NULL)
    {
        //Copying the fields to the currentElement and nullfies the other fields like parent, sibling, child.
        currentElement = CreateDocument(currentElement, -5, "\0");
        strcpy(currentElement->fileName, head->fileName);
        currentElement->relevancy = head->relevancy;

        //Inserts that element to the header of our heap.
        Header = InsertToBinomHeap(Header, currentElement);

        //Recursively calls the function to traverse the heap.
        InsertElementsOf(head->child);
        head = head->sibling;
    }
}