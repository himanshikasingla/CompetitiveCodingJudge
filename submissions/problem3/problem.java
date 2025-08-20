import java.util.Scanner; // Import the Scanner class for user input

public class problem {

    public static void main(String[] args) {
        Scanner input = new Scanner(System.in); // Create a Scanner object

        int size = input.nextInt(); // Read the size of the array

        int[] numbers = new int[size]; // Declare and initialize the array

        for (int i = 0; i < size; i++)
        {
            numbers[i] = input.nextInt(); // Read each element from the user
        }

        int sum = 0; // Initialize sum to 0
        for (int i = 0; i < size; i++) 
         {
            sum += numbers[i]; // Add each element to the sum
        }

        System.out.println(sum);

        input.close(); // Close the scanner to release resources
    }
}